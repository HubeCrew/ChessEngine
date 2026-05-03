#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import re
import shlex
import subprocess
import sys
import time
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Iterable

try:
    import chess
except ImportError as error:  # pragma: no cover - exercised by users without the tool venv.
    print(
        "missing dependency: python-chess. Install with "
        "`python3 -m venv .venv && .venv/bin/pip install -r requirements-tools.txt`.",
        file=sys.stderr,
    )
    raise SystemExit(2) from error


PIECE_VALUES = {
    chess.PAWN: 100,
    chess.KNIGHT: 320,
    chess.BISHOP: 330,
    chess.ROOK: 500,
    chess.QUEEN: 900,
    chess.KING: 0,
}

TRACE_COMPONENTS = [
    "material",
    "piece_square",
    "mobility",
    "safe_mobility",
    "king_safety",
    "threats",
    "pawn_structure",
    "outposts",
    "rook_files",
    "space",
    "center_control",
    "bishop_quality",
    "pawn_dynamics",
    "development",
    "trade_context",
    "total",
]

RESULT_TOKENS = {"1-0", "0-1", "1/2-1/2", "*"}


@dataclass
class GameRecord:
    path: Path
    headers: dict[str, str]
    moves: list[str]


@dataclass
class MoveRecord:
    game: int
    ply: int
    move_number: int
    side: str
    mover: str
    opponent: str
    opening: str
    result: str
    termination: str
    uci: str
    san: str
    fen_before: str
    fen_after: str
    score_before_cp: int
    score_after_cp: int
    delta_cp: int
    trace_before: dict[str, int]
    trace_after: dict[str, int]
    component_delta: dict[str, int]
    moving_piece: str
    captured_piece: str = ""
    capture_value: int = 0
    moving_value: int = 0
    is_capture: bool = False
    is_promotion: bool = False
    gives_check: bool = False
    is_castling: bool = False
    is_queen_trade: bool = False
    is_equal_trade: bool = False
    reply_uci: str = ""
    reply_san: str = ""
    score_after_reply_cp: int | None = None
    sequence_delta_cp: int | None = None
    engine_recaptures_available: list[str] = field(default_factory=list)
    categories: list[str] = field(default_factory=list)
    severity: int = 0
    engine_bestmove: str = ""
    engine_bestmove_score_cp: int | None = None


class UciTraceEngine:
    def __init__(self, command: str, timeout: float) -> None:
        self.command = shlex.split(command)
        self.timeout = timeout
        self.process: subprocess.Popen[str] | None = None
        self.cache: dict[str, dict[str, int]] = {}

    def __enter__(self) -> "UciTraceEngine":
        self.process = subprocess.Popen(
            self.command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self._send("uci")
        self._read_until("uciok")
        self._send("isready")
        self._read_until("readyok")
        return self

    def __exit__(self, *_: object) -> None:
        if self.process is None:
            return
        try:
            if self.process.poll() is None:
                self._send("quit")
                self.process.wait(timeout=1)
        except Exception:
            self.process.kill()

    def trace(self, fen: str) -> dict[str, int]:
        cached = self.cache.get(fen)
        if cached is not None:
            return cached
        self._send(f"position fen {fen}")
        self._send("eval")
        deadline = time.monotonic() + self.timeout
        while True:
            line = self._readline(deadline)
            if line.startswith("info string eval "):
                trace = parse_trace_line(line)
                self.cache[fen] = trace
                return trace

    def bestmove(self, fen: str, depth: int) -> tuple[str, int | None]:
        self._send(f"position fen {fen}")
        self._send(f"go depth {depth}")
        deadline = time.monotonic() + self.timeout
        latest_score: int | None = None
        while True:
            line = self._readline(deadline)
            if line.startswith("info ") and " score cp " in line:
                parts = line.split()
                try:
                    latest_score = int(parts[parts.index("cp") + 1])
                except (ValueError, IndexError):
                    pass
            if line.startswith("bestmove "):
                parts = line.split()
                return (parts[1] if len(parts) > 1 else "0000", latest_score)

    def _send(self, command: str) -> None:
        if self.process is None or self.process.stdin is None or self.process.poll() is not None:
            raise RuntimeError("engine is not running")
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def _read_until(self, expected: str) -> None:
        deadline = time.monotonic() + self.timeout
        while self._readline(deadline) != expected:
            pass

    def _readline(self, deadline: float) -> str:
        if self.process is None or self.process.stdout is None:
            raise RuntimeError("engine is not running")
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("engine protocol timeout")
        while time.monotonic() < deadline:
            line = self.process.stdout.readline()
            if line:
                return line.strip()
            if self.process.poll() is not None:
                stderr = self.process.stderr.read() if self.process.stderr is not None else ""
                raise RuntimeError(f"engine exited with code {self.process.returncode}: {stderr}")
        raise TimeoutError("engine protocol timeout")


def parse_trace_line(line: str) -> dict[str, int]:
    parts = line.split()
    values: dict[str, int] = {}
    index = 3  # info string eval
    while index + 1 < len(parts):
        key = parts[index]
        value = parts[index + 1]
        if key in TRACE_COMPONENTS:
            values[key] = int(value)
        index += 2
    missing = [key for key in TRACE_COMPONENTS if key not in values]
    if missing:
        raise ValueError(f"engine eval output is missing components: {missing}: {line}")
    return values


def parse_pgn(path: Path) -> GameRecord:
    text = path.read_text(encoding="utf-8")
    headers = dict(re.findall(r'^\[([A-Za-z0-9_]+)\s+"(.*)"\]$', text, flags=re.MULTILINE))
    body_lines = [line for line in text.splitlines() if line and not line.startswith("[")]
    body = " ".join(body_lines)
    body = re.sub(r"\{[^}]*\}", " ", body)
    body = re.sub(r"\([^)]*\)", " ", body)
    moves: list[str] = []
    for token in body.split():
        if token in RESULT_TOKENS:
            continue
        if token.endswith(".") or re.match(r"^\d+\.+$", token):
            continue
        if re.match(r"^\d+\.\.\.$", token):
            continue
        if re.match(r"^[a-h][1-8][a-h][1-8][nbrq]?$", token):
            moves.append(token)
    if "FEN" not in headers:
        raise ValueError(f"{path}: missing FEN header")
    return GameRecord(path=path, headers=headers, moves=moves)


def pgn_paths(input_path: Path) -> list[Path]:
    if input_path.is_file():
        return [input_path]
    return sorted(input_path.glob("game-*.pgn"))


def color_name(color: chess.Color) -> str:
    return "white" if color == chess.WHITE else "black"


def engine_won(result: str, side: chess.Color) -> bool:
    return (result == "1-0" and side == chess.WHITE) or (result == "0-1" and side == chess.BLACK)


def engine_lost(result: str, side: chess.Color) -> bool:
    return (result == "0-1" and side == chess.WHITE) or (result == "1-0" and side == chess.BLACK)


def material_value(piece: chess.Piece | None) -> int:
    return 0 if piece is None else PIECE_VALUES[piece.piece_type]


def captured_piece_for(board: chess.Board, move: chess.Move) -> chess.Piece | None:
    if board.is_en_passant(move):
        offset = -8 if board.turn == chess.WHITE else 8
        return board.piece_at(move.to_square + offset)
    return board.piece_at(move.to_square)


def trace_for_side(trace: dict[str, int], side: chess.Color) -> int:
    return trace["total"] if side == chess.WHITE else -trace["total"]


def component_deltas(before: dict[str, int], after: dict[str, int], side: chess.Color) -> dict[str, int]:
    sign = 1 if side == chess.WHITE else -1
    return {
        key: sign * (after[key] - before[key])
        for key in TRACE_COMPONENTS
        if key != "total"
    }


def classify_move(
    record: MoveRecord,
    previous_record: MoveRecord | None,
    game_lost_by_engine: bool,
    swing_threshold: int,
    trade_drop_threshold: int,
) -> None:
    if game_lost_by_engine:
        record.categories.append("engine-loss")
    if record.delta_cp <= -swing_threshold:
        record.categories.append("eval-swing")
    if record.is_capture and record.capture_value <= record.moving_value and record.delta_cp <= -trade_drop_threshold:
        record.categories.append("low-value-capture-drop")
    if record.is_capture and abs(record.capture_value - record.moving_value) <= 80:
        record.categories.append("equal-value-capture")
    if record.is_queen_trade:
        record.categories.append("queen-trade")
    if record.is_equal_trade:
        record.categories.append("recapture-trade")
    if record.is_capture and record.component_delta.get("king_safety", 0) <= -25:
        record.categories.append("capture-hurts-king-safety")
    if record.is_capture and record.component_delta.get("trade_context", 0) <= -20:
        record.categories.append("capture-hurts-trade-context")
    if previous_record is not None and previous_record.is_capture and record.is_capture:
        if previous_record.captured_piece == record.moving_piece and record.captured_piece == previous_record.moving_piece:
            record.categories.append("trade-sequence")
    if not record.categories and game_lost_by_engine and record.ply >= 8:
        record.categories.append("loss-context")

    record.severity = max(0, -record.delta_cp)
    if "eval-swing" in record.categories:
        record.severity += 120
    if "queen-trade" in record.categories:
        record.severity += 80
    if "low-value-capture-drop" in record.categories:
        record.severity += 80
    if "capture-hurts-king-safety" in record.categories:
        record.severity += 60
    if "capture-hurts-trade-context" in record.categories:
        record.severity += 50
    if "engine-loss" in record.categories:
        record.severity += 25


def add_category(record: MoveRecord, category: str, severity_bonus: int = 0) -> None:
    if category not in record.categories:
        record.categories.append(category)
    record.severity += severity_bonus


def remove_category(record: MoveRecord, category: str) -> None:
    if category in record.categories:
        record.categories.remove(category)


def analyze_game(
    game: GameRecord,
    engine_name: str,
    trace_engine: UciTraceEngine,
    swing_threshold: int,
    trade_drop_threshold: int,
    analyze_both_sides: bool,
) -> list[MoveRecord]:
    board = chess.Board(game.headers["FEN"])
    white = game.headers.get("White", "white")
    black = game.headers.get("Black", "black")
    result = game.headers.get("Result", "*")
    opening = game.headers.get("Opening", "")
    termination = game.headers.get("Termination", "")
    game_number = int(game.headers.get("Round", "0") or "0")
    previous_engine_record: MoveRecord | None = None
    records: list[MoveRecord] = []

    for ply_index, uci in enumerate(game.moves):
        side = board.turn
        mover = white if side == chess.WHITE else black
        opponent = black if side == chess.WHITE else white
        move = chess.Move.from_uci(uci)
        if move not in board.legal_moves:
            raise ValueError(f"{game.path}: illegal move at ply {ply_index + 1}: {uci}")

        should_analyze = analyze_both_sides or mover == engine_name
        fen_before = board.fen()
        moving_piece = board.piece_at(move.from_square)
        captured_piece = captured_piece_for(board, move) if board.is_capture(move) else None
        san = board.san(move)
        is_capture = board.is_capture(move)
        gives_check = board.gives_check(move)
        is_castling = board.is_castling(move)
        is_promotion = move.promotion is not None
        recaptures_previous_engine_move = (
            not should_analyze
            and previous_engine_record is not None
            and previous_engine_record.ply == ply_index
            and is_capture
            and captured_piece is not None
            and moving_piece is not None
            and captured_piece.symbol().upper() == previous_engine_record.moving_piece
        )
        if recaptures_previous_engine_move:
            previous_engine_record.is_equal_trade = (
                abs(material_value(moving_piece) - previous_engine_record.capture_value) <= 80
                or abs(material_value(captured_piece) - previous_engine_record.moving_value) <= 80
            )
            previous_engine_record.reply_uci = uci
            previous_engine_record.reply_san = san
            add_category(previous_engine_record, "opponent-recapture", 80)
            if previous_engine_record.is_equal_trade:
                add_category(previous_engine_record, "recapture-trade", 70)
            if moving_piece.piece_type == chess.QUEEN or captured_piece.piece_type == chess.QUEEN:
                add_category(previous_engine_record, "queen-trade-sequence", 90)
        trace_before = trace_engine.trace(fen_before) if should_analyze else {}
        score_before = trace_for_side(trace_before, side) if should_analyze else 0
        board.push(move)
        fen_after = board.fen()
        if recaptures_previous_engine_move and previous_engine_record is not None:
            reply_trace = trace_engine.trace(fen_after)
            sign = 1 if previous_engine_record.side == "white" else -1
            previous_engine_record.score_after_reply_cp = sign * reply_trace["total"]
            previous_engine_record.sequence_delta_cp = (
                previous_engine_record.score_after_reply_cp - previous_engine_record.score_before_cp
            )
            previous_engine_record.engine_recaptures_available = [
                legal.uci()
                for legal in board.legal_moves
                if board.is_capture(legal) and legal.to_square == move.to_square
            ]
            if previous_engine_record.engine_recaptures_available:
                add_category(previous_engine_record, "engine-recapture-available", 20)
                remove_category(previous_engine_record, "bad-trade-sequence")
            elif previous_engine_record.sequence_delta_cp <= -trade_drop_threshold:
                add_category(previous_engine_record, "bad-trade-sequence", 120 + -previous_engine_record.sequence_delta_cp)
        trace_after = trace_engine.trace(fen_after) if should_analyze else {}
        score_after = trace_for_side(trace_after, side) if should_analyze else 0

        if should_analyze:
            game_lost = engine_lost(result, side) if mover == engine_name else False
            record = MoveRecord(
                game=game_number,
                ply=ply_index + 1,
                move_number=ply_index // 2 + 1,
                side=color_name(side),
                mover=mover,
                opponent=opponent,
                opening=opening,
                result=result,
                termination=termination,
                uci=uci,
                san=san,
                fen_before=fen_before,
                fen_after=fen_after,
                score_before_cp=score_before,
                score_after_cp=score_after,
                delta_cp=score_after - score_before,
                trace_before=trace_before,
                trace_after=trace_after,
                component_delta=component_deltas(trace_before, trace_after, side),
                moving_piece=moving_piece.symbol().upper() if moving_piece is not None else "",
                captured_piece=captured_piece.symbol().upper() if captured_piece is not None else "",
                capture_value=material_value(captured_piece),
                moving_value=material_value(moving_piece),
                is_capture=is_capture,
                is_promotion=is_promotion,
                gives_check=gives_check,
                is_castling=is_castling,
                is_queen_trade=moving_piece is not None
                and captured_piece is not None
                and moving_piece.piece_type == chess.QUEEN
                and captured_piece.piece_type == chess.QUEEN,
            )
            if previous_engine_record is not None:
                record.is_equal_trade = (
                    previous_engine_record.is_capture
                    and record.is_capture
                    and abs(previous_engine_record.capture_value - record.capture_value) <= 80
                )
            classify_move(record, previous_engine_record, game_lost, swing_threshold, trade_drop_threshold)
            records.append(record)
            previous_engine_record = record

    return records


def select_events(records: list[MoveRecord], max_events: int) -> list[MoveRecord]:
    categorized = [
        record
        for record in records
        if record.categories
        and (
            "loss-context" not in record.categories
            or record.delta_cp <= -50
            or record.ply >= max(1, max(r.ply for r in records if r.game == record.game) - 10)
        )
    ]

    def top_matching(predicate: object, limit: int) -> list[MoveRecord]:
        matched = [record for record in categorized if predicate(record)]
        matched.sort(key=lambda record: (record.severity, -record.ply), reverse=True)
        return matched[:limit]

    trade_categories = {
        "equal-value-capture",
        "queen-trade",
        "queen-trade-sequence",
        "recapture-trade",
        "opponent-recapture",
        "low-value-capture-drop",
        "bad-trade-sequence",
        "engine-recapture-available",
        "capture-hurts-king-safety",
        "capture-hurts-trade-context",
    }
    chosen: list[MoveRecord] = []
    chosen.extend(top_matching(lambda record: "eval-swing" in record.categories, max(8, max_events // 3)))
    chosen.extend(top_matching(lambda record: bool(trade_categories & set(record.categories)), max(12, max_events // 3)))
    chosen.extend(top_matching(lambda record: "engine-loss" in record.categories, max(8, max_events // 4)))
    categorized.sort(key=lambda record: (record.severity, -record.ply), reverse=True)
    chosen.extend(categorized)

    deduped: list[MoveRecord] = []
    seen: set[tuple[int, int, str]] = set()
    for record in chosen:
        key = (record.game, record.ply, record.uci)
        if key in seen:
            continue
        seen.add(key)
        deduped.append(record)
        if len(deduped) >= max_events:
            break
    return deduped


def summarize(records: list[MoveRecord], events: list[MoveRecord], engine_name: str) -> dict[str, object]:
    game_ids = sorted({record.game for record in records})
    loss_ids = sorted({record.game for record in records if "engine-loss" in record.categories})
    return {
        "engine": engine_name,
        "games_seen": len(game_ids),
        "engine_moves_analyzed": len(records),
        "flagged_events": len(events),
        "loss_games_seen": len(loss_ids),
        "average_delta_cp": round(sum(record.delta_cp for record in records) / max(1, len(records)), 2),
        "worst_delta_cp": min((record.delta_cp for record in records), default=0),
        "captures": sum(1 for record in records if record.is_capture),
        "equal_value_captures": sum(1 for record in records if "equal-value-capture" in record.categories),
        "queen_trades": sum(1 for record in records if "queen-trade" in record.categories),
        "eval_swings": sum(1 for record in records if "eval-swing" in record.categories),
    }


def write_json(path: Path, summary: dict[str, object], events: list[MoveRecord]) -> None:
    payload = {
        "summary": summary,
        "events": [asdict(event) for event in events],
    }
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def write_csv(path: Path, events: list[MoveRecord]) -> None:
    fields = [
        "severity",
        "categories",
        "game",
        "ply",
        "move_number",
        "side",
        "mover",
        "result",
        "opening",
        "uci",
        "san",
        "score_before_cp",
        "score_after_cp",
        "delta_cp",
        "reply_uci",
        "reply_san",
        "score_after_reply_cp",
        "sequence_delta_cp",
        "moving_piece",
        "captured_piece",
        "capture_value",
        "fen_before",
    ]
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for event in events:
            row = {field_name: getattr(event, field_name) for field_name in fields if field_name != "categories"}
            row["categories"] = "|".join(event.categories)
            writer.writerow(row)


def write_epd(path: Path, events: list[MoveRecord]) -> None:
    lines = [
        "# Positions flagged by gauntlet_postmortem.py.",
        "# bm is the move played in the gauntlet, not necessarily the correct move.",
    ]
    for index, event in enumerate(events, start=1):
        board = chess.Board(event.fen_before)
        epd = " ".join(event.fen_before.split()[:4])
        lines.append(
            f'{epd} bm {event.uci}; id "postmortem-{index:04d}-g{event.game}-p{event.ply}"; '
            f'theme "{"|".join(event.categories)}"; acd 5; '
            f'hmvc {board.halfmove_clock}; fmvn {board.fullmove_number}; '
            f'c0 "{event.mover} played {event.uci} ({event.san}), delta {event.delta_cp} cp";'
        )
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def format_trace_delta(event: MoveRecord) -> str:
    ranked = sorted(event.component_delta.items(), key=lambda item: item[1])
    worst = [f"{key} {value:+d}" for key, value in ranked[:4] if value != 0]
    best = [f"{key} {value:+d}" for key, value in ranked[-2:] if value > 0]
    return ", ".join(worst + best) if worst or best else "no component movement"


def write_markdown(path: Path, summary: dict[str, object], events: list[MoveRecord]) -> None:
    lines = [
        "# Gauntlet Postmortem",
        "",
        "## Summary",
        "",
    ]
    for key, value in summary.items():
        lines.append(f"- `{key}`: {value}")
    def append_event_list(title: str, selected: list[MoveRecord], limit: int) -> None:
        lines.extend(["", f"## {title}", ""])
        for index, event in enumerate(selected[:limit], start=1):
            lines.extend(
                [
                    f"### {index}. Game {event.game}, ply {event.ply}: {event.uci} ({event.san})",
                    "",
                    f"- Categories: `{', '.join(event.categories)}`",
                    f"- Score: `{event.score_before_cp:+d} -> {event.score_after_cp:+d}` "
                    f"for {event.mover}, delta `{event.delta_cp:+d}`",
                    (
                        f"- Sequence after reply `{event.reply_uci}`: `{event.score_before_cp:+d} -> "
                        f"{event.score_after_reply_cp:+d}`, delta `{event.sequence_delta_cp:+d}`"
                        if event.sequence_delta_cp is not None and event.score_after_reply_cp is not None
                        else "- Sequence after reply: `n/a`"
                    ),
                    (
                        f"- Engine recaptures available: `{', '.join(event.engine_recaptures_available)}`"
                        if event.engine_recaptures_available
                        else "- Engine recaptures available: `none`"
                    ),
                    f"- Material: `{event.moving_piece}` captures `{event.captured_piece or '-'}` "
                    f"for `{event.capture_value}` cp",
                    f"- Component movement: {format_trace_delta(event)}",
                    f"- Result: `{event.result}` `{event.termination}`",
                    f"- FEN before: `{event.fen_before}`",
                    "",
                ]
            )
            if event.engine_bestmove:
                lines.append(f"- Engine re-search: `{event.engine_bestmove}` score `{event.engine_bestmove_score_cp}`")
                lines.append("")

    trade_categories = {
        "equal-value-capture",
        "queen-trade",
        "queen-trade-sequence",
        "recapture-trade",
        "opponent-recapture",
        "low-value-capture-drop",
        "bad-trade-sequence",
        "engine-recapture-available",
        "capture-hurts-king-safety",
        "capture-hurts-trade-context",
    }
    append_event_list("Highest-Severity Events", events, 25)
    append_event_list(
        "Trade And Simplification Watchlist",
        [event for event in events if trade_categories & set(event.categories)],
        25,
    )
    append_event_list(
        "Loss-Game Watchlist",
        [event for event in events if "engine-loss" in event.categories],
        20,
    )
    append_event_list(
        "Largest Eval Swings",
        [event for event in events if "eval-swing" in event.categories],
        20,
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def add_engine_bestmoves(events: list[MoveRecord], engine: UciTraceEngine, depth: int) -> None:
    if depth <= 0:
        return
    for event in events:
        bestmove, score = engine.bestmove(event.fen_before, depth)
        event.engine_bestmove = bestmove
        event.engine_bestmove_score_cp = score


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Analyze gauntlet PGNs for bad trades and evaluation swings.")
    parser.add_argument("--pgn-dir", required=True, type=Path, help="Gauntlet output directory or one PGN file.")
    parser.add_argument("--engine", default="./build-release/chess_uci", help="Engine command used for eval traces.")
    parser.add_argument("--engine-name", required=True, help="Engine name as it appears in PGN White/Black headers.")
    parser.add_argument("--output-dir", type=Path, default=Path("runs/postmortems/latest"))
    parser.add_argument("--max-events", type=int, default=80)
    parser.add_argument("--swing-threshold", type=int, default=120)
    parser.add_argument("--trade-drop-threshold", type=int, default=50)
    parser.add_argument("--engine-depth", type=int, default=0, help="Optional depth for bestmove re-search on flagged events.")
    parser.add_argument("--both-sides", action="store_true", help="Analyze both players instead of only --engine-name moves.")
    parser.add_argument("--protocol-timeout", type=float, default=10.0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    paths = pgn_paths(args.pgn_dir)
    if not paths:
        print(f"no PGN files found in {args.pgn_dir}", file=sys.stderr)
        return 2
    if args.max_events <= 0:
        print("--max-events must be positive", file=sys.stderr)
        return 2

    args.output_dir.mkdir(parents=True, exist_ok=True)
    records: list[MoveRecord] = []
    with UciTraceEngine(args.engine, args.protocol_timeout) as trace_engine:
        for path in paths:
            game = parse_pgn(path)
            records.extend(
                analyze_game(
                    game,
                    args.engine_name,
                    trace_engine,
                    args.swing_threshold,
                    args.trade_drop_threshold,
                    args.both_sides,
                )
            )
        events = select_events(records, args.max_events)
        add_engine_bestmoves(events, trace_engine, args.engine_depth)

    summary = summarize(records, events, args.engine_name)
    write_json(args.output_dir / "postmortem.json", summary, events)
    write_csv(args.output_dir / "events.csv", events)
    write_epd(args.output_dir / "positions.epd", events)
    write_markdown(args.output_dir / "report.md", summary, events)

    print(f"games_seen {summary['games_seen']}")
    print(f"engine_moves_analyzed {summary['engine_moves_analyzed']}")
    print(f"flagged_events {summary['flagged_events']}")
    print(f"report {args.output_dir / 'report.md'}")
    print(f"events {args.output_dir / 'events.csv'}")
    print(f"positions {args.output_dir / 'positions.epd'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
