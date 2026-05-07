import { useEffect, useMemo, useState } from "react";
import type { ReactNode } from "react";
import {
  ArrowsClockwise,
  BookOpenText,
  Brain,
  Copy,
  Cpu,
  Database,
  FlagCheckered,
  GearSix,
  Lightning,
  Play,
  Stop,
  Swap,
  WarningCircle
} from "@phosphor-icons/react";
import { getState, newGame, playMove, resetGame, stopEngine } from "./api";
import type { Color, EngineOptionSpec, GameState, NewGamePayload, SearchLine, TimeMode } from "./types";

type Piece = {
  code: string;
  color: Color;
  label: string;
};

type SetupForm = {
  humanColor: Color;
  mode: TimeMode;
  baseMinutes: number;
  incrementSeconds: number;
  movesToGo: number;
  moveTimeSeconds: number;
  depth: number;
  engineOptions: Record<string, string | number | boolean>;
};

type SetupFormModel = {
  initial: SetupForm;
  initialKey: string;
};

const pieceNames: Record<string, string> = {
  p: "P",
  n: "N",
  b: "B",
  r: "R",
  q: "Q",
  k: "K"
};

const optionGroups = [
  {
    title: "Search",
    icon: Cpu,
    names: ["Threads", "Hash", "MultiPV", "Move Overhead", "Slow Mover", "NullMovePruning", "SearchExtensions"]
  },
  {
    title: "Evaluation",
    icon: Brain,
    names: ["EvalType", "NnueFile"]
  },
  {
    title: "Book",
    icon: BookOpenText,
    names: ["OwnBook", "BookFile", "BestBookMove", "BookDepth", "BookMinimumWeight"]
  },
  {
    title: "Tablebase",
    icon: Database,
    names: ["SyzygyPath", "SyzygyProbeDepth"]
  }
];

function App() {
  const [state, setState] = useState<GameState | null>(null);
  const [loading, setLoading] = useState(true);
  const [requesting, setRequesting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [flipped, setFlipped] = useState(false);
  const [selected, setSelected] = useState<string | null>(null);
  const [promotion, setPromotion] = useState<{ from: string; to: string; moves: string[] } | null>(null);

  useEffect(() => {
    let cancelled = false;
    getState()
      .then((next) => {
        if (!cancelled) {
          setState(next);
          setFlipped(next.humanColor === "black");
        }
      })
      .catch((reason) => setError(String(reason)))
      .finally(() => {
        if (!cancelled) setLoading(false);
      });
    return () => {
      cancelled = true;
    };
  }, []);

  useEffect(() => {
    const source = new EventSource("/api/events");
    const applyPayload = (event: MessageEvent) => {
      const data = JSON.parse(event.data);
      if (data && typeof data === "object" && "fen" in data) {
        setState(data as GameState);
      } else if (data?.state) {
        setState(data.state as GameState);
      }
    };
    source.addEventListener("state", applyPayload);
    source.addEventListener("tick", applyPayload);
    source.addEventListener("search", applyPayload);
    source.addEventListener("error", applyPayload);
    source.onerror = () => setError("Live connection lost. The app will keep trying to reconnect.");
    return () => source.close();
  }, []);

  const form = useSetupForm(state);
  const boardPieces = useMemo(() => parseFen(state?.fen ?? ""), [state?.fen]);
  const lastMove = state?.moves.at(-1);
  const humanTurn = state?.status === "playing" && state.turn === state.humanColor && !state.engineSearching;

  async function startGame(payload: NewGamePayload) {
    setRequesting(true);
    setError(null);
    try {
      const next = await newGame(payload);
      setState(next);
      setSelected(null);
      setPromotion(null);
      setFlipped(next.humanColor === "black");
    } catch (reason) {
      setError(String(reason));
    } finally {
      setRequesting(false);
    }
  }

  async function sendMove(uci: string) {
    setRequesting(true);
    setError(null);
    try {
      const next = await playMove(uci);
      setState(next);
      setSelected(null);
      setPromotion(null);
    } catch (reason) {
      setError(String(reason));
    } finally {
      setRequesting(false);
    }
  }

  async function reset() {
    setRequesting(true);
    try {
      const next = await resetGame();
      setState(next);
      setSelected(null);
      setPromotion(null);
    } finally {
      setRequesting(false);
    }
  }

  async function stop() {
    setRequesting(true);
    try {
      setState(await stopEngine());
    } finally {
      setRequesting(false);
    }
  }

  function onSquare(square: string) {
    if (!state || !humanTurn || promotion) return;
    const piece = boardPieces.get(square);
    if (!selected) {
      if (piece?.color === state.humanColor) setSelected(square);
      return;
    }
    const candidates = state.legalMoves.filter((move) => move.slice(0, 2) === selected && move.slice(2, 4) === square);
    if (candidates.length === 1) {
      void sendMove(candidates[0]);
      return;
    }
    if (candidates.length > 1) {
      setPromotion({ from: selected, to: square, moves: candidates });
      return;
    }
    if (piece?.color === state.humanColor) {
      setSelected(square);
    } else {
      setSelected(null);
    }
  }

  const shell = "mx-auto grid min-h-[100dvh] w-full max-w-[1480px] grid-cols-1 gap-5 px-4 py-4 md:px-6 xl:grid-cols-[minmax(0,1.08fr)_430px]";

  if (loading) {
    return (
      <main className={shell}>
        <div className="skeleton min-h-[720px] rounded-lg border border-line" />
        <div className="skeleton min-h-[720px] rounded-lg border border-line" />
      </main>
    );
  }

  return (
    <main className={shell}>
      <section className="grid content-start gap-4">
        <Header state={state} onReset={reset} onStop={stop} onFlip={() => setFlipped((value) => !value)} requesting={requesting} />
        {(error || state?.lastError || state?.engineError) && (
          <div className="flex items-start gap-3 rounded-lg border border-red-300/70 bg-red-50 px-4 py-3 text-sm text-red-900">
            <WarningCircle size={20} weight="duotone" />
            <div>{error ?? state?.lastError ?? state?.engineError}</div>
          </div>
        )}
        <div className="grid gap-4 lg:grid-cols-[minmax(0,1fr)_320px]">
          <BoardPanel
            state={state}
            pieces={boardPieces}
            flipped={flipped}
            selected={selected}
            lastMove={lastMove?.uci}
            humanTurn={humanTurn}
            legalTargets={selected ? legalTargets(state, selected) : new Set<string>()}
            onSquare={onSquare}
          />
          <TelemetryPanel state={state} />
        </div>
      </section>
      <aside className="grid content-start gap-4">
        {form && <SetupPanel state={state} form={form} onStart={startGame} requesting={requesting} />}
        <MoveList moves={state?.moves ?? []} />
      </aside>
      {promotion && (
        <PromotionDialog
          color={state?.humanColor ?? "white"}
          moves={promotion.moves}
          onPick={(uci) => void sendMove(uci)}
          onCancel={() => setPromotion(null)}
        />
      )}
    </main>
  );
}

function Header({
  state,
  onReset,
  onStop,
  onFlip,
  requesting
}: {
  state: GameState | null;
  onReset: () => void;
  onStop: () => void;
  onFlip: () => void;
  requesting: boolean;
}) {
  const statusText = state?.engineSearching ? "Engine thinking" : state?.status === "playing" ? `${state.turn} to move` : state?.status ?? "setup";
  return (
    <header className="grid gap-4 border-b border-line pb-4 md:grid-cols-[minmax(0,1fr)_auto] md:items-end">
      <div>
        <div className="mb-3 inline-flex items-center gap-2 rounded-full border border-line bg-white/70 px-3 py-1 text-xs font-semibold uppercase tracking-[0.12em] text-coal/70 shadow-sm">
          <span className={`h-2 w-2 rounded-full bg-moss ${state?.engineSearching ? "breathing-dot" : ""}`} />
          {statusText}
        </div>
        <h1 className="max-w-[11ch] text-4xl font-[760] leading-none tracking-tight text-ink md:text-6xl">Engine board</h1>
        <p className="mt-3 max-w-[70ch] text-base leading-relaxed text-coal/66">
          Play against the release UCI build with live PV, time controls, book, NNUE, Syzygy, and search configuration in one surface.
        </p>
      </div>
      <div className="flex flex-wrap gap-2 md:justify-end">
        <IconButton title="Flip board" onClick={onFlip} icon={<Swap size={18} />} />
        <IconButton title="Reset game" onClick={onReset} icon={<ArrowsClockwise size={18} />} disabled={requesting} />
        <IconButton title="Stop engine" onClick={onStop} icon={<Stop size={18} />} disabled={!state?.engineSearching || requesting} />
      </div>
    </header>
  );
}

function BoardPanel({
  state,
  pieces,
  flipped,
  selected,
  lastMove,
  humanTurn,
  legalTargets,
  onSquare
}: {
  state: GameState | null;
  pieces: Map<string, Piece>;
  flipped: boolean;
  selected: string | null;
  lastMove?: string;
  humanTurn: boolean;
  legalTargets: Set<string>;
  onSquare: (square: string) => void;
}) {
  const squares = boardSquares(flipped);
  return (
    <article className="rounded-lg border border-line bg-white/76 p-3 shadow-diffusion backdrop-blur md:p-5">
      <div className="mb-4 grid gap-3 sm:grid-cols-2">
        <PlayerClock label="Black" active={state?.turn === "black"} clock={state?.blackClockMs ?? 0} side="black" engine={state?.engineColor === "black"} />
        <PlayerClock label="White" active={state?.turn === "white"} clock={state?.whiteClockMs ?? 0} side="white" engine={state?.engineColor === "white"} />
      </div>
      <div className="grid aspect-square w-full grid-cols-8 overflow-hidden rounded-lg border border-coal/20 bg-coal/20">
        {squares.map((square) => {
          const piece = pieces.get(square);
          const isLight = squareIsLight(square);
          const isLast = lastMove?.slice(0, 2) === square || lastMove?.slice(2, 4) === square;
          const canMove = legalTargets.has(square);
          const canCapture = canMove && Boolean(piece);
          return (
            <button
              key={square}
              type="button"
              aria-label={square}
              onClick={() => onSquare(square)}
              className={[
                "relative grid min-h-0 min-w-0 place-items-center transition-transform duration-200 active:scale-[0.985]",
                isLight ? "square-light" : "square-dark",
                isLast ? "square-last" : "",
                selected === square ? "square-selected" : "",
                canMove && !canCapture ? "legal-dot" : "",
                canCapture ? "legal-capture" : "",
                humanTurn ? "cursor-pointer" : "cursor-default"
              ].join(" ")}
            >
              {piece && <PieceView piece={piece} />}
              <Coordinate square={square} flipped={flipped} />
            </button>
          );
        })}
      </div>
      <div className="mt-4 grid gap-3 border-t border-line pt-4 md:grid-cols-[1fr_auto] md:items-center">
        <div className="min-w-0 font-mono text-sm text-coal/70">
          <span className="text-coal/45">FEN</span> <span className="break-all text-coal">{state?.fen}</span>
        </div>
        <button
          type="button"
          onClick={() => state?.fen && navigator.clipboard?.writeText(state.fen)}
          className="inline-flex items-center justify-center gap-2 rounded-md border border-line bg-paper px-3 py-2 text-sm font-semibold text-coal transition duration-200 active:translate-y-[1px] active:scale-[0.99]"
        >
          <Copy size={16} /> Copy FEN
        </button>
      </div>
    </article>
  );
}

function PieceView({ piece }: { piece: Piece }) {
  return <span className={`piece-disc piece-${piece.color}`}>{piece.label}</span>;
}

function Coordinate({ square, flipped }: { square: string; flipped: boolean }) {
  const file = square[0];
  const rank = square[1];
  const showFile = flipped ? rank === "8" : rank === "1";
  const showRank = flipped ? file === "h" : file === "a";
  return (
    <>
      {showRank && <span className="absolute left-1.5 top-1 font-mono text-[10px] font-bold text-coal/45">{rank}</span>}
      {showFile && <span className="absolute bottom-1 right-1.5 font-mono text-[10px] font-bold text-coal/45">{file}</span>}
    </>
  );
}

function PlayerClock({ label, clock, active, side, engine }: { label: string; clock: number; active: boolean; side: Color; engine: boolean }) {
  return (
    <div className={`grid grid-cols-[1fr_auto] items-center gap-3 rounded-md border px-3 py-3 ${active ? "border-moss bg-moss/9" : "border-line bg-paper/70"}`}>
      <div>
        <div className="text-sm font-bold text-ink">{label}</div>
        <div className="text-xs uppercase tracking-[0.12em] text-coal/48">{engine ? "Engine" : "Human"} {side}</div>
      </div>
      <div className="font-mono text-xl font-bold tabular-nums text-ink">{formatClock(clock)}</div>
    </div>
  );
}

function TelemetryPanel({ state }: { state: GameState | null }) {
  const search = state?.lastSearch;
  const percent = evalPercent(search);
  return (
    <article className="grid content-start gap-4 rounded-lg border border-line bg-white/68 p-4 shadow-diffusion backdrop-blur">
      <PanelTitle icon={<Lightning size={18} />} title="Search trace" note={state?.enginePath ?? "build-release/chess_uci"} />
      <div className="grid grid-cols-[auto_1fr] items-center gap-4 border-y border-line py-4">
        <div className="font-mono text-2xl font-bold text-ink">{scoreLabel(search)}</div>
        <div className="h-3 overflow-hidden rounded-full border border-line bg-coal/12">
          <div className="h-full rounded-full bg-moss transition-all duration-500" style={{ width: `${percent}%` }} />
        </div>
      </div>
      <div className="grid grid-cols-2 gap-x-4 gap-y-3 border-b border-line pb-4 font-mono text-sm">
        <Metric label="Depth" value={search?.depth ?? "--"} />
        <Metric label="Nodes" value={formatCompact(search?.nodes)} />
        <Metric label="NPS" value={formatCompact(search?.nps)} />
        <Metric label="Time" value={search?.time ? `${search.time}ms` : "--"} />
      </div>
      <div>
        <div className="mb-2 text-xs font-bold uppercase tracking-[0.14em] text-coal/48">Principal variation</div>
        {search?.pv?.length ? (
          <div className="rounded-md bg-coal px-3 py-3 font-mono text-sm leading-relaxed text-paper">{search.pv.join(" ")}</div>
        ) : (
          <EmptyLine text="PV appears after the engine starts searching." />
        )}
      </div>
      <div className="grid gap-2">
        <div className="text-xs font-bold uppercase tracking-[0.14em] text-coal/48">Recent lines</div>
        {state?.lastInfoLines?.length ? state.lastInfoLines.map((line, index) => <SearchRow key={`${line.depth}-${index}`} line={line} />) : <EmptyLine text="No search lines yet." />}
      </div>
    </article>
  );
}

function SetupPanel({
  state,
  form,
  onStart,
  requesting
}: {
  state: GameState | null;
  form: SetupFormModel;
  onStart: (payload: NewGamePayload) => void;
  requesting: boolean;
}) {
  const [local, setLocal] = useState(form.initial);

  useEffect(() => {
    setLocal(form.initial);
  }, [form.initialKey]);

  function updateOption(name: string, value: string | number | boolean) {
    setLocal((current) => ({
      ...current,
      engineOptions: { ...current.engineOptions, [name]: value }
    }));
  }

  const payload: NewGamePayload = {
    humanColor: local.humanColor,
    timeControl: {
      mode: local.mode,
      baseMs: minutesToMs(local.baseMinutes),
      incrementMs: secondsToMs(local.incrementSeconds),
      movesToGo: local.movesToGo,
      moveTimeMs: secondsToMs(local.moveTimeSeconds),
      depth: local.depth
    },
    engineOptions: local.engineOptions
  };

  return (
    <section className="rounded-lg border border-line bg-white/76 p-4 shadow-diffusion backdrop-blur">
      <PanelTitle icon={<GearSix size={18} />} title="Game setup" note={state?.status === "playing" ? "Changes apply on new game" : "Ready"} />
      <div className="mt-4 grid gap-5">
        <div className="grid gap-2">
          <Label text="Side" helper="Choose your color before starting the game." />
          <div className="grid grid-cols-2 gap-2 rounded-lg bg-paper p-1">
            {(["white", "black"] as Color[]).map((color) => (
              <button
                key={color}
                type="button"
                onClick={() => setLocal((current) => ({ ...current, humanColor: color }))}
                className={`rounded-md px-3 py-2 text-sm font-bold capitalize transition duration-200 active:translate-y-[1px] ${local.humanColor === color ? "bg-coal text-paper" : "text-coal/68 hover:bg-white"}`}
              >
                {color}
              </button>
            ))}
          </div>
        </div>
        <div className="grid gap-2">
          <Label text="Time control" helper="Clock mode sends wtime, btime, increment, and optional movestogo." />
          <div className="grid grid-cols-3 gap-2 rounded-lg bg-paper p-1">
            {(["clock", "movetime", "depth"] as TimeMode[]).map((mode) => (
              <button
                key={mode}
                type="button"
                onClick={() => setLocal((current) => ({ ...current, mode }))}
                className={`rounded-md px-2 py-2 text-sm font-bold capitalize transition duration-200 active:translate-y-[1px] ${local.mode === mode ? "bg-coal text-paper" : "text-coal/68 hover:bg-white"}`}
              >
                {mode === "movetime" ? "Move time" : mode}
              </button>
            ))}
          </div>
          {local.mode === "clock" && (
            <div className="grid grid-cols-3 gap-3">
              <NumberField label="Base min" value={local.baseMinutes} min={0} onChange={(baseMinutes) => setLocal((current) => ({ ...current, baseMinutes }))} />
              <NumberField label="Inc sec" value={local.incrementSeconds} min={0} onChange={(incrementSeconds) => setLocal((current) => ({ ...current, incrementSeconds }))} />
              <NumberField label="Moves to go" value={local.movesToGo} min={0} onChange={(movesToGo) => setLocal((current) => ({ ...current, movesToGo }))} />
            </div>
          )}
          {local.mode === "movetime" && <NumberField label="Move time sec" value={local.moveTimeSeconds} min={0.1} step={0.1} onChange={(moveTimeSeconds) => setLocal((current) => ({ ...current, moveTimeSeconds }))} />}
          {local.mode === "depth" && <NumberField label="Depth" value={local.depth} min={1} max={127} onChange={(depth) => setLocal((current) => ({ ...current, depth }))} />}
        </div>
        {optionGroups.map((group) => {
          const Icon = group.icon;
          return (
            <div key={group.title} className="border-t border-line pt-4">
              <div className="mb-3 flex items-center gap-2 text-sm font-bold text-ink">
                <Icon size={17} /> {group.title}
              </div>
              <div className="grid gap-3">
                {group.names.map((name) => (
                  <OptionInput
                    key={name}
                    name={name}
                    spec={state?.engineOptionSpecs[name]}
                    value={local.engineOptions[name]}
                    suggestions={state?.suggestions}
                    onChange={(value) => updateOption(name, value)}
                  />
                ))}
              </div>
            </div>
          );
        })}
        <button
          type="button"
          onClick={() => onStart(payload)}
          disabled={requesting}
          className="inline-flex min-h-12 items-center justify-center gap-2 rounded-lg bg-coal px-4 py-3 text-sm font-bold text-paper shadow-diffusion transition duration-200 hover:bg-ink active:translate-y-[1px] active:scale-[0.99] disabled:cursor-not-allowed disabled:opacity-55"
        >
          <Play size={18} weight="fill" /> Start new game
        </button>
      </div>
    </section>
  );
}

function MoveList({ moves }: { moves: GameState["moves"] }) {
  const rows = [];
  for (let index = 0; index < moves.length; index += 2) {
    rows.push({ moveNo: index / 2 + 1, white: moves[index], black: moves[index + 1] });
  }
  return (
    <section className="rounded-lg border border-line bg-white/68 p-4 shadow-diffusion backdrop-blur">
      <PanelTitle icon={<FlagCheckered size={18} />} title="Move record" note={`${moves.length} plies`} />
      <div className="mt-3 max-h-[420px] overflow-auto border-t border-line pt-2">
        {rows.length ? (
          rows.map((row) => (
            <div key={row.moveNo} className="grid min-h-9 grid-cols-[42px_1fr_1fr] items-center gap-2 border-b border-line/70 font-mono text-sm">
              <span className="text-coal/44">{row.moveNo}.</span>
              <MoveCell move={row.white} />
              <MoveCell move={row.black} />
            </div>
          ))
        ) : (
          <EmptyLine text="Moves will appear here after the first legal move." />
        )}
      </div>
    </section>
  );
}

function MoveCell({ move }: { move?: GameState["moves"][number] }) {
  if (!move) return <span />;
  return <span className={`truncate rounded px-2 py-1 ${move.by === "engine" ? "bg-moss/14 text-ink" : "text-coal"}`}>{move.san}</span>;
}

function PromotionDialog({ color, moves, onPick, onCancel }: { color: Color; moves: string[]; onPick: (uci: string) => void; onCancel: () => void }) {
  const choices = ["q", "r", "b", "n"].filter((piece) => moves.some((move) => move.endsWith(piece)));
  return (
    <div className="fixed inset-0 grid place-items-center bg-coal/36 px-4 backdrop-blur-sm">
      <div className="w-full max-w-sm rounded-lg border border-white/20 bg-paper p-5 shadow-diffusion">
        <div className="text-lg font-bold text-ink">Choose promotion</div>
        <div className="mt-4 grid grid-cols-4 gap-2">
          {choices.map((piece) => (
            <button
              key={piece}
              type="button"
              onClick={() => onPick(moves.find((move) => move.endsWith(piece))!)}
              className="grid aspect-square place-items-center rounded-md border border-line bg-white transition duration-200 active:scale-[0.98]"
            >
              <span className={`piece-disc piece-${color}`}>{pieceNames[piece]}</span>
            </button>
          ))}
        </div>
        <button type="button" onClick={onCancel} className="mt-4 w-full rounded-md border border-line px-3 py-2 text-sm font-bold text-coal transition active:translate-y-[1px]">
          Cancel
        </button>
      </div>
    </div>
  );
}

function OptionInput({
  name,
  spec,
  value,
  suggestions,
  onChange
}: {
  name: string;
  spec?: EngineOptionSpec;
  value: string | number | boolean;
  suggestions?: GameState["suggestions"];
  onChange: (value: string | number | boolean) => void;
}) {
  if (!spec) return null;
  const id = `option-${name.replace(/\s+/g, "-")}`;
  if (spec.type === "check") {
    return (
      <label className="flex items-center justify-between gap-3 rounded-md border border-line bg-paper/70 px-3 py-2">
        <span className="text-sm font-semibold text-coal">{name}</span>
        <input type="checkbox" checked={Boolean(value)} onChange={(event) => onChange(event.target.checked)} className="h-5 w-5 accent-moss" />
      </label>
    );
  }
  if (spec.type === "combo") {
    return (
      <div className="grid gap-2">
        <Label text={name} />
        <select id={id} value={String(value)} onChange={(event) => onChange(event.target.value)} className="min-h-10 rounded-md border border-line bg-paper px-3 text-sm outline-none focus:border-moss">
          {spec.vars.map((item) => (
            <option key={item} value={item}>
              {item}
            </option>
          ))}
        </select>
      </div>
    );
  }
  if (spec.type === "spin") {
    return <NumberField label={name} value={Number(value)} min={spec.min} max={spec.max} onChange={onChange} />;
  }
  const list = name === "NnueFile" ? "nnue-files" : name === "BookFile" ? "book-files" : name === "SyzygyPath" ? "syzygy-dirs" : undefined;
  return (
    <div className="grid gap-2">
      <Label text={name} />
      <input
        id={id}
        value={String(value)}
        list={list}
        onChange={(event) => onChange(event.target.value)}
        placeholder="<empty>"
        className="min-h-10 rounded-md border border-line bg-paper px-3 font-mono text-sm outline-none transition focus:border-moss"
      />
      {name === "NnueFile" && <Datalist id="nnue-files" values={suggestions?.nnueFiles ?? []} />}
      {name === "BookFile" && <Datalist id="book-files" values={suggestions?.bookFiles ?? []} />}
      {name === "SyzygyPath" && <Datalist id="syzygy-dirs" values={suggestions?.syzygyDirs ?? []} />}
    </div>
  );
}

function Datalist({ id, values }: { id: string; values: string[] }) {
  return (
    <datalist id={id}>
      {values.map((value) => (
        <option key={value} value={value} />
      ))}
    </datalist>
  );
}

function NumberField({
  label,
  value,
  min,
  max,
  step = 1,
  onChange
}: {
  label: string;
  value: number;
  min: number;
  max?: number;
  step?: number;
  onChange: (value: number) => void;
}) {
  return (
    <div className="grid gap-2">
      <Label text={label} />
      <input
        type="number"
        value={value}
        min={min}
        max={max}
        step={step}
        onChange={(event) => onChange(Number(event.target.value))}
        className="min-h-10 min-w-0 rounded-md border border-line bg-paper px-3 font-mono text-sm outline-none transition focus:border-moss"
      />
    </div>
  );
}

function Label({ text, helper }: { text: string; helper?: string }) {
  return (
    <label className="grid gap-1">
      <span className="text-sm font-bold text-coal">{text}</span>
      {helper && <span className="text-xs leading-relaxed text-coal/52">{helper}</span>}
    </label>
  );
}

function PanelTitle({ icon, title, note }: { icon: ReactNode; title: string; note?: string }) {
  return (
    <div className="flex items-start justify-between gap-3">
      <div className="flex items-center gap-2 text-sm font-bold uppercase tracking-[0.14em] text-coal/62">
        {icon} {title}
      </div>
      {note && <div className="max-w-[18rem] truncate text-right font-mono text-xs text-coal/45">{note}</div>}
    </div>
  );
}

function IconButton({ title, icon, onClick, disabled }: { title: string; icon: ReactNode; onClick: () => void; disabled?: boolean }) {
  return (
    <button
      type="button"
      title={title}
      aria-label={title}
      onClick={onClick}
      disabled={disabled}
      className="inline-flex h-10 w-10 items-center justify-center rounded-md border border-line bg-white/74 text-coal shadow-sm transition duration-200 hover:bg-paper active:translate-y-[1px] active:scale-[0.98] disabled:cursor-not-allowed disabled:opacity-45"
    >
      {icon}
    </button>
  );
}

function Metric({ label, value }: { label: string; value: ReactNode }) {
  return (
    <div>
      <div className="text-xs uppercase tracking-[0.12em] text-coal/42">{label}</div>
      <div className="mt-1 font-mono font-bold text-ink">{value}</div>
    </div>
  );
}

function EmptyLine({ text }: { text: string }) {
  return <div className="rounded-md border border-dashed border-line bg-paper/50 px-3 py-3 text-sm leading-relaxed text-coal/56">{text}</div>;
}

function SearchRow({ line }: { line: SearchLine }) {
  return (
    <div className="grid grid-cols-[48px_64px_1fr] gap-2 border-b border-line/60 py-2 font-mono text-xs text-coal">
      <span>D{line.depth ?? "--"}</span>
      <span>{scoreLabel(line)}</span>
      <span className="truncate text-coal/58">{line.pv?.join(" ") || line.raw}</span>
    </div>
  );
}

function useSetupForm(state: GameState | null): SetupFormModel | null {
  return useMemo(() => {
    if (!state) return null;
    const control = state.timeControl;
    const initial: SetupForm = {
      humanColor: state.humanColor,
      mode: control.mode,
      baseMinutes: Math.max(0, Math.round(control.base_ms / 60000)),
      incrementSeconds: Math.max(0, Math.round(control.increment_ms / 1000)),
      movesToGo: control.movestogo,
      moveTimeSeconds: Math.max(0.1, control.movetime_ms / 1000),
      depth: control.depth,
      engineOptions: state.engineOptions
    };
    return {
      initial,
      initialKey: JSON.stringify(initial)
    };
  }, [state]);
}

function parseFen(fen: string): Map<string, Piece> {
  const pieces = new Map<string, Piece>();
  const placement = fen.split(" ")[0];
  if (!placement) return pieces;
  const ranks = placement.split("/");
  for (let rankIndex = 0; rankIndex < ranks.length; rankIndex += 1) {
    const rank = 8 - rankIndex;
    let fileIndex = 0;
    for (const char of ranks[rankIndex]) {
      if (/\d/.test(char)) {
        fileIndex += Number(char);
        continue;
      }
      const file = String.fromCharCode(97 + fileIndex);
      const lower = char.toLowerCase();
      pieces.set(`${file}${rank}`, {
        code: char,
        color: char === lower ? "black" : "white",
        label: pieceNames[lower] ?? lower.toUpperCase()
      });
      fileIndex += 1;
    }
  }
  return pieces;
}

function boardSquares(flipped: boolean): string[] {
  const files = flipped ? ["h", "g", "f", "e", "d", "c", "b", "a"] : ["a", "b", "c", "d", "e", "f", "g", "h"];
  const ranks = flipped ? [1, 2, 3, 4, 5, 6, 7, 8] : [8, 7, 6, 5, 4, 3, 2, 1];
  return ranks.flatMap((rank) => files.map((file) => `${file}${rank}`));
}

function squareIsLight(square: string): boolean {
  const file = square.charCodeAt(0) - 97;
  const rank = Number(square[1]);
  return (file + rank) % 2 === 0;
}

function legalTargets(state: GameState | null, from: string): Set<string> {
  return new Set((state?.legalMoves ?? []).filter((move) => move.startsWith(from)).map((move) => move.slice(2, 4)));
}

function minutesToMs(value: number): number {
  return Math.max(0, Math.round(value * 60000));
}

function secondsToMs(value: number): number {
  return Math.max(0, Math.round(value * 1000));
}

function formatClock(ms: number): string {
  const total = Math.max(0, Math.ceil(ms / 1000));
  const minutes = Math.floor(total / 60);
  const seconds = total % 60;
  return `${minutes}:${seconds.toString().padStart(2, "0")}`;
}

function formatCompact(value?: number): string {
  if (value === undefined) return "--";
  return Intl.NumberFormat(undefined, { notation: "compact", maximumFractionDigits: 1 }).format(value);
}

function scoreLabel(search?: SearchLine): string {
  if (!search || search.score === undefined) return "--";
  if (search.scoreType === "mate") return `M${search.score}`;
  const score = Number(search.score) / 100;
  return `${score >= 0 ? "+" : ""}${score.toFixed(2)}`;
}

function evalPercent(search?: SearchLine): number {
  if (!search || search.score === undefined) return 50;
  if (search.scoreType === "mate") return Number(search.score) > 0 ? 96 : 4;
  return Math.max(4, Math.min(96, 50 + Number(search.score) / 18));
}

export default App;
