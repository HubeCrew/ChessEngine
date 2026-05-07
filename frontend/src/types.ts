export type Color = "white" | "black";

export type TimeMode = "clock" | "movetime" | "depth";

export type TimeControl = {
  mode: TimeMode;
  base_ms: number;
  increment_ms: number;
  movestogo: number;
  movetime_ms: number;
  depth: number;
};

export type EngineOptionSpec =
  | { type: "spin"; default: number; min: number; max: number }
  | { type: "check"; default: boolean }
  | { type: "string"; default: string }
  | { type: "combo"; default: string; vars: string[] };

export type MoveRecord = {
  ply: number;
  san: string;
  uci: string;
  fen: string;
  by: "human" | "engine";
};

export type SearchLine = {
  raw?: string;
  depth?: number;
  multipv?: number;
  scoreType?: "cp" | "mate" | string;
  score?: number;
  nodes?: number;
  qnodes?: number;
  nps?: number;
  time?: number;
  pv?: string[];
};

export type EngineSuggestions = {
  nnueFiles: string[];
  bookFiles: string[];
  syzygyDirs: string[];
};

export type GameState = {
  status: "setup" | "playing" | "gameover" | "error";
  enginePath: string;
  engineReady: boolean;
  engineSearching: boolean;
  engineError: string | null;
  lastError: string | null;
  fen: string;
  turn: Color;
  humanColor: Color;
  engineColor: Color;
  legalMoves: string[];
  moves: MoveRecord[];
  result: string;
  outcome: string | null;
  whiteClockMs: number;
  blackClockMs: number;
  timeControl: TimeControl;
  engineOptions: Record<string, string | number | boolean>;
  engineOptionSpecs: Record<string, EngineOptionSpec>;
  lastSearch: SearchLine;
  lastInfoLines: SearchLine[];
  suggestions: EngineSuggestions;
};

export type NewGamePayload = {
  humanColor: Color;
  timeControl: {
    mode: TimeMode;
    baseMs: number;
    incrementMs: number;
    movesToGo: number;
    moveTimeMs: number;
    depth: number;
  };
  engineOptions: Record<string, string | number | boolean>;
};
