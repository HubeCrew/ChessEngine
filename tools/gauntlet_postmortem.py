#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import queue
import re
import shlex
import subprocess
import sys
import threading
import time
from collections import deque
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Iterable

try:
    import chess
    import chess.polyglot
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
MATE_SCORE_CP = 900_000


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
    engine_bestmove_pv: str = ""
    engine_played_score_cp: int | None = None
    engine_played_see_cp: int | None = None
    engine_played_pv: str = ""
    engine_reference_score_cp: int | None = None
    engine_reference_see_cp: int | None = None
    engine_reference_delta_cp: int | None = None
    engine_reference_pv: str = ""
    reference_bestmove: str = ""
    reference_score_cp: int | None = None
    reference_pv: str = ""
    reference_played_score_cp: int | None = None
    reference_played_pv: str = ""
    reference_delta_cp: int | None = None
    reference_avoid: bool = False
    reference_scan_depth: int = 0
    reference_scan_movetime_ms: int = 0
    first_reference_blunder: bool = False
    in_book: bool = False
    played_book_move: bool = False
    book_bestmove: str = ""
    book_move_count: int = 0
    book_total_weight: int = 0
    book_played_weight: int = 0
    legal_moves: int = 0
    piece_count: int = 0
    phase: str = ""


@dataclass(frozen=True)
class SearchResult:
    bestmove: str
    score_cp: int | None
    pv: tuple[str, ...]


class UciTraceEngine:
    def __init__(self, command: str, timeout: float, options: Iterable[tuple[str, str]] = ()) -> None:
        self.command = shlex.split(command)
        self.timeout = timeout
        self.options = list(options)
        self.process: subprocess.Popen[str] | None = None
        self.lines: queue.Queue[str | None] = queue.Queue()
        self.reader: threading.Thread | None = None
        self.stderr_reader: threading.Thread | None = None
        self.stdout_tail: deque[str] = deque(maxlen=80)
        self.stderr_tail: deque[str] = deque(maxlen=120)
        self.cache: dict[str, dict[str, int]] = {}
        self.search_cache: dict[tuple[str, int, int, tuple[str, ...]], SearchResult] = {}
        self.see_cache: dict[tuple[str, str], int] = {}

    def __enter__(self) -> "UciTraceEngine":
        self.process = subprocess.Popen(
            self.command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        if self.process.stdout is None or self.process.stdin is None:
            raise RuntimeError("failed to open engine pipes")
        self.reader = threading.Thread(target=self._read_stdout, daemon=True)
        self.reader.start()
        if self.process.stderr is not None:
            self.stderr_reader = threading.Thread(target=self._read_stderr, daemon=True)
            self.stderr_reader.start()
        self._send("uci")
        self._read_until("uciok")
        for name, value in self.options:
            self._send(f"setoption name {name} value {value}")
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

    def search(
        self,
        fen: str,
        depth: int = 0,
        movetime_ms: int = 0,
        search_moves: Iterable[str] = (),
    ) -> SearchResult:
        search_move_tuple = tuple(search_moves)
        cache_key = (fen, depth, movetime_ms, search_move_tuple)
        cached = self.search_cache.get(cache_key)
        if cached is not None:
            return cached
        self._send(f"position fen {fen}")
        search_moves_clause = " ".join(search_move_tuple)
        if movetime_ms > 0:
            command = f"go movetime {movetime_ms}"
        else:
            command = f"go depth {depth}"
        if search_moves_clause:
            command += f" searchmoves {search_moves_clause}"
        self._send(command)
        deadline = time.monotonic() + self.timeout
        latest_score: int | None = None
        latest_pv: tuple[str, ...] = ()
        while True:
            line = self._readline(deadline)
            if line.startswith("info ") and " score " in line:
                parts = line.split()
                parsed_score = parse_uci_score(parts)
                if parsed_score is not None:
                    latest_score = parsed_score
                latest_pv = parse_uci_pv(parts) or latest_pv
            if line.startswith("bestmove "):
                parts = line.split()
                result = SearchResult(parts[1] if len(parts) > 1 else "0000", latest_score, latest_pv)
                self.search_cache[cache_key] = result
                return result

    def bestmove(
        self,
        fen: str,
        depth: int = 0,
        movetime_ms: int = 0,
        search_moves: Iterable[str] = (),
    ) -> tuple[str, int | None]:
        result = self.search(fen, depth, movetime_ms, search_moves)
        return result.bestmove, result.score_cp

    def see(self, fen: str, move: str) -> int:
        cache_key = (fen, move)
        cached = self.see_cache.get(cache_key)
        if cached is not None:
            return cached
        self._send(f"position fen {fen}")
        self._send(f"see {move}")
        deadline = time.monotonic() + self.timeout
        while True:
            line = self._readline(deadline)
            if line.startswith("info string see "):
                parts = line.split()
                if len(parts) < 5:
                    raise ValueError(f"invalid see output: {line}")
                score = int(parts[4])
                self.see_cache[cache_key] = score
                return score

    def _send(self, command: str) -> None:
        if self.process is None or self.process.stdin is None or self.process.poll() is not None:
            raise self._engine_error("engine is not running")
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def _read_until(self, expected: str) -> None:
        deadline = time.monotonic() + self.timeout
        while self._readline(deadline) != expected:
            pass

    def _readline(self, deadline: float) -> str:
        if self.process is None:
            raise self._engine_error("engine is not running")
        if self.process.poll() is not None:
            raise self._engine_error(f"engine exited with code {self.process.returncode}")
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise self._engine_error("engine protocol timeout")
        try:
            line = self.lines.get(timeout=remaining)
        except queue.Empty:
            raise self._engine_error("engine protocol timeout")
        if line is None:
            raise self._engine_error("engine closed stdout")
        return line

    def _read_stdout(self) -> None:
        assert self.process is not None
        assert self.process.stdout is not None
        for line in self.process.stdout:
            stripped = line.strip()
            self.stdout_tail.append(stripped)
            self.lines.put(stripped)
        self.lines.put(None)

    def _read_stderr(self) -> None:
        assert self.process is not None
        assert self.process.stderr is not None
        for line in self.process.stderr:
            self.stderr_tail.append(line.rstrip())

    def _engine_error(self, message: str) -> RuntimeError:
        exit_code = None if self.process is None else self.process.poll()
        details = [message]
        if exit_code is not None:
            details.append(f"exit_code={exit_code}")
        if self.stderr_tail:
            details.append("stderr_tail=" + " | ".join(self.stderr_tail))
        if self.stdout_tail:
            details.append("stdout_tail=" + " | ".join(self.stdout_tail))
        return RuntimeError("; ".join(details))


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


def parse_uci_score(parts: list[str]) -> int | None:
    try:
        score_index = parts.index("score")
        score_type = parts[score_index + 1]
        score_value = int(parts[score_index + 2])
    except (ValueError, IndexError):
        return None
    if score_type == "cp":
        return score_value
    if score_type == "mate":
        magnitude = MATE_SCORE_CP - min(abs(score_value), 10_000)
        return magnitude if score_value > 0 else -magnitude
    return None


def parse_uci_pv(parts: list[str]) -> tuple[str, ...]:
    try:
        pv_index = parts.index("pv")
    except ValueError:
        return ()
    return tuple(part for part in parts[pv_index + 1 :] if part)


def format_pv(pv: tuple[str, ...] | str) -> str:
    if isinstance(pv, str):
        return pv
    return " ".join(pv)


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


def piece_count(board: chess.Board) -> int:
    return sum(1 for _square, _piece in board.piece_map().items())


def game_phase(board: chess.Board) -> str:
    pieces = piece_count(board)
    queens = len(board.pieces(chess.QUEEN, chess.WHITE)) + len(board.pieces(chess.QUEEN, chess.BLACK))
    minor_major = sum(
        len(board.pieces(piece_type, chess.WHITE)) + len(board.pieces(piece_type, chess.BLACK))
        for piece_type in (chess.KNIGHT, chess.BISHOP, chess.ROOK, chess.QUEEN)
    )
    if pieces <= 10 or (queens == 0 and minor_major <= 6):
        return "endgame"
    if pieces <= 22 or queens == 0:
        return "middlegame/endgame"
    return "opening/middlegame"


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
        legal_moves_before = board.legal_moves.count()
        piece_count_before = piece_count(board)
        phase_before = game_phase(board)
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
                legal_moves=legal_moves_before,
                piece_count=piece_count_before,
                phase=phase_before,
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
    scored_reference_events = [
        event for event in events
        if event.engine_reference_delta_cp is not None
    ]
    played_see_events = [
        event for event in events
        if event.engine_played_see_cp is not None
    ]
    reference_see_events = [
        event for event in events
        if event.engine_reference_see_cp is not None
    ]
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
        "engine_reference_scores": len(scored_reference_events),
        "engine_prefers_reference": sum(1 for event in scored_reference_events if event.engine_reference_delta_cp > 0),
        "engine_prefers_played": sum(1 for event in scored_reference_events if event.engine_reference_delta_cp < 0),
        "engine_ties_reference": sum(1 for event in scored_reference_events if event.engine_reference_delta_cp == 0),
        "engine_played_negative_see": sum(1 for event in played_see_events if event.engine_played_see_cp < 0),
        "engine_reference_negative_see": sum(1 for event in reference_see_events if event.engine_reference_see_cp < 0),
        "engine_prefers_negative_see": sum(
            1 for event in events if "engine-prefers-negative-see" in event.categories
        ),
        "engine_can_avoid_negative_see": sum(
            1 for event in events if "engine-can-avoid-negative-see" in event.categories
        ),
        "reference_scored_moves": sum(1 for record in records if record.reference_delta_cp is not None),
        "reference_blunders": sum(1 for record in records if "reference-blunder" in record.categories),
        "reference_inaccuracies": sum(1 for record in records if "reference-inaccuracy" in record.categories),
        "first_reference_blunders": sum(1 for record in records if record.first_reference_blunder),
        "book_positions": sum(1 for record in records if record.in_book),
        "book_deviations": sum(1 for record in records if "book-deviation" in record.categories),
        "book_weaker_lines": sum(1 for record in records if "book-chose-weaker-line" in record.categories),
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
        "engine_recaptures_available",
        "moving_piece",
        "captured_piece",
        "capture_value",
        "engine_bestmove",
        "engine_bestmove_score_cp",
        "engine_bestmove_pv",
        "engine_played_score_cp",
        "engine_played_see_cp",
        "engine_played_pv",
        "engine_reference_score_cp",
        "engine_reference_see_cp",
        "engine_reference_delta_cp",
        "engine_reference_pv",
        "reference_bestmove",
        "reference_score_cp",
        "reference_pv",
        "reference_played_score_cp",
        "reference_played_pv",
        "reference_delta_cp",
        "reference_avoid",
        "reference_scan_depth",
        "reference_scan_movetime_ms",
        "first_reference_blunder",
        "in_book",
        "played_book_move",
        "book_bestmove",
        "book_move_count",
        "book_total_weight",
        "book_played_weight",
        "legal_moves",
        "piece_count",
        "phase",
        "fen_before",
    ]
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for event in events:
            row = {field_name: getattr(event, field_name) for field_name in fields if field_name != "categories"}
            row["categories"] = "|".join(event.categories)
            row["engine_recaptures_available"] = "|".join(event.engine_recaptures_available)
            writer.writerow(row)


def write_game_summary_csv(path: Path, records: list[MoveRecord]) -> None:
    fields = [
        "game",
        "opening",
        "result",
        "termination",
        "engine_side",
        "engine_moves",
        "worst_eval_delta_cp",
        "worst_eval_ply",
        "worst_eval_move",
        "worst_reference_delta_cp",
        "worst_reference_ply",
        "worst_reference_move",
        "first_reference_blunder_ply",
        "first_reference_blunder_move",
        "first_reference_blunder_delta_cp",
        "book_positions",
        "book_deviations",
        "first_out_of_book_ply",
    ]
    grouped: dict[int, list[MoveRecord]] = {}
    for record in records:
        grouped.setdefault(record.game, []).append(record)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for game, game_records in sorted(grouped.items()):
            game_records.sort(key=lambda record: record.ply)
            worst_eval = min(game_records, key=lambda record: record.delta_cp)
            ref_scored = [record for record in game_records if record.reference_delta_cp is not None]
            worst_ref = max(ref_scored, key=lambda record: record.reference_delta_cp or 0) if ref_scored else None
            first_ref = next((record for record in game_records if record.first_reference_blunder), None)
            first_out_of_book = next((record for record in game_records if not record.in_book), None)
            first = game_records[0]
            writer.writerow(
                {
                    "game": game,
                    "opening": first.opening,
                    "result": first.result,
                    "termination": first.termination,
                    "engine_side": ",".join(sorted({record.side for record in game_records})),
                    "engine_moves": len(game_records),
                    "worst_eval_delta_cp": worst_eval.delta_cp,
                    "worst_eval_ply": worst_eval.ply,
                    "worst_eval_move": worst_eval.uci,
                    "worst_reference_delta_cp": "" if worst_ref is None else worst_ref.reference_delta_cp,
                    "worst_reference_ply": "" if worst_ref is None else worst_ref.ply,
                    "worst_reference_move": "" if worst_ref is None else worst_ref.uci,
                    "first_reference_blunder_ply": "" if first_ref is None else first_ref.ply,
                    "first_reference_blunder_move": "" if first_ref is None else first_ref.uci,
                    "first_reference_blunder_delta_cp": "" if first_ref is None else first_ref.reference_delta_cp,
                    "book_positions": sum(1 for record in game_records if record.in_book),
                    "book_deviations": sum(1 for record in game_records if "book-deviation" in record.categories),
                    "first_out_of_book_ply": "" if first_out_of_book is None else first_out_of_book.ply,
                }
            )


def write_epd(path: Path, events: list[MoveRecord]) -> None:
    lines = [
        "# Positions flagged by gauntlet_postmortem.py.",
        "# bm is the optional reference-engine move. am is the move played in the gauntlet and should be avoided.",
    ]
    for index, event in enumerate(events, start=1):
        board = chess.Board(event.fen_before)
        epd = " ".join(event.fen_before.split()[:4])
        strict_reference = bool(event.reference_bestmove) and event.reference_bestmove == event.uci
        best_move = f"bm {event.reference_bestmove}; " if strict_reference else ""
        avoid_move = f"am {event.uci}; " if not event.reference_bestmove or event.reference_avoid else ""
        reference_comment = (
            f' c1 "reference {event.reference_bestmove} score {event.reference_score_cp} '
            f'played_score {event.reference_played_score_cp} delta {event.reference_delta_cp}";'
            if event.reference_bestmove and not strict_reference
            else ""
        )
        lines.append(
            f'{epd} {best_move}{avoid_move}id "postmortem-{index:04d}-g{event.game}-p{event.ply}"; '
            f'theme "{"|".join(event.categories)}"; acd 5; '
            f'hmvc {board.halfmove_clock}; fmvn {board.fullmove_number}; '
            f'c0 "{event.mover} played {event.uci} ({event.san}), delta {event.delta_cp} cp";'
            f'{reference_comment}'
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
                    f"- Position: `{event.phase}`, pieces `{event.piece_count}`, legal moves `{event.legal_moves}`",
                    (
                        f"- Book: in book, played `{event.uci}` weight `{event.book_played_weight}`, "
                        f"best `{event.book_bestmove}`, choices `{event.book_move_count}`"
                        if event.in_book
                        else "- Book: out of book"
                    ),
                    f"- Component movement: {format_trace_delta(event)}",
                    f"- Result: `{event.result}` `{event.termination}`",
                    f"- FEN before: `{event.fen_before}`",
                    "",
                ]
            )
            if event.engine_bestmove:
                lines.append(f"- Engine re-search: `{event.engine_bestmove}` score `{event.engine_bestmove_score_cp}`")
                if event.engine_bestmove_pv:
                    lines.append(f"- Engine re-search PV: `{event.engine_bestmove_pv}`")
                lines.append("")
            if event.engine_played_score_cp is not None:
                lines.append(f"- Engine played-move constrained score: `{event.engine_played_score_cp}`")
                if event.engine_played_pv:
                    lines.append(f"- Engine played-move PV: `{event.engine_played_pv}`")
                if event.engine_played_see_cp is not None:
                    lines.append(f"- Engine played-move SEE: `{event.engine_played_see_cp}`")
                if event.engine_reference_score_cp is not None and event.engine_reference_delta_cp is not None:
                    lines.append(
                        f"- Engine reference-move constrained score: `{event.engine_reference_score_cp}`, "
                        f"delta `{event.engine_reference_delta_cp:+d}`"
                    )
                    if event.engine_reference_pv:
                        lines.append(f"- Engine reference-move PV: `{event.engine_reference_pv}`")
                    if event.engine_reference_see_cp is not None:
                        lines.append(f"- Engine reference-move SEE: `{event.engine_reference_see_cp}`")
                lines.append("")
            if event.reference_bestmove:
                lines.append(f"- Reference bestmove: `{event.reference_bestmove}` score `{event.reference_score_cp}`")
                if event.reference_pv:
                    lines.append(f"- Reference PV: `{event.reference_pv}`")
                if event.reference_played_score_cp is not None and event.reference_delta_cp is not None:
                    lines.append(
                        f"- Reference played-move score: `{event.reference_played_score_cp}`, "
                        f"delta `{event.reference_delta_cp:+d}`"
                    )
                    if event.reference_played_pv:
                        lines.append(f"- Reference played-move PV: `{event.reference_played_pv}`")
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
        "Negative SEE Capture Audit",
        [event for event in events if "played-negative-see" in event.categories],
        25,
    )
    append_event_list(
        "First Reference Blunder By Game",
        [event for event in events if event.first_reference_blunder],
        25,
    )
    append_event_list(
        "Reference Blunder Scan",
        [event for event in events if "reference-blunder" in event.categories],
        25,
    )
    append_event_list(
        "Book Exit And Book Deviation Watchlist",
        [event for event in events if "book-deviation" in event.categories or "book-chose-weaker-line" in event.categories],
        25,
    )
    append_event_list(
        "Engine Prefers Negative SEE",
        [event for event in events if "engine-prefers-negative-see" in event.categories],
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
        best = engine.search(event.fen_before, depth)
        event.engine_bestmove = best.bestmove
        event.engine_bestmove_score_cp = best.score_cp
        event.engine_bestmove_pv = format_pv(best.pv)
        played = engine.search(event.fen_before, depth, search_moves=(event.uci,))
        event.engine_played_score_cp = played.score_cp
        event.engine_played_pv = format_pv(played.pv)
        event.engine_played_see_cp = engine.see(event.fen_before, event.uci)


def add_engine_reference_scores(events: list[MoveRecord], engine: UciTraceEngine, depth: int) -> None:
    if depth <= 0:
        return
    for event in events:
        if not event.reference_bestmove or event.reference_bestmove == "0000":
            continue
        board = chess.Board(event.fen_before)
        reference_move = chess.Move.from_uci(event.reference_bestmove)
        if reference_move not in board.legal_moves:
            continue
        reference = engine.search(event.fen_before, depth, search_moves=(event.reference_bestmove,))
        event.engine_reference_score_cp = reference.score_cp
        event.engine_reference_pv = format_pv(reference.pv)
        event.engine_reference_see_cp = engine.see(event.fen_before, event.reference_bestmove)
        if event.engine_played_score_cp is not None and reference.score_cp is not None:
            event.engine_reference_delta_cp = reference.score_cp - event.engine_played_score_cp


def add_engine_diagnostic_categories(events: list[MoveRecord]) -> None:
    for event in events:
        if event.engine_played_see_cp is not None and event.engine_played_see_cp < 0:
            add_category(event, "played-negative-see", min(80, -event.engine_played_see_cp // 4))
        if event.engine_reference_see_cp is not None and event.engine_reference_see_cp < 0:
            add_category(event, "reference-negative-see")

        if event.engine_played_see_cp is None or event.engine_played_see_cp >= 0:
            continue
        if event.engine_reference_delta_cp is None:
            continue

        reference_see = event.engine_reference_see_cp if event.engine_reference_see_cp is not None else 0
        if event.engine_reference_delta_cp <= -30 and reference_see >= event.engine_played_see_cp:
            add_category(event, "engine-prefers-negative-see", 90)
        elif event.engine_reference_delta_cp >= 30:
            add_category(event, "engine-can-avoid-negative-see", 60)


def add_reference_diagnostic_categories(records: list[MoveRecord], min_delta_cp: int) -> None:
    first_by_game: dict[int, MoveRecord] = {}
    for record in records:
        if record.reference_delta_cp is None:
            continue
        if record.reference_avoid:
            add_category(record, "reference-blunder", 150 + min(500, record.reference_delta_cp))
            first_by_game.setdefault(record.game, record)
        elif record.reference_delta_cp >= max(50, min_delta_cp // 2):
            add_category(record, "reference-inaccuracy", 60 + min(240, record.reference_delta_cp))
    for record in first_by_game.values():
        record.first_reference_blunder = True
        add_category(record, "first-reference-blunder", 250)


def annotate_book(
    records: list[MoveRecord],
    book_file: Path | None,
    minimum_weight: int,
    book_depth: int,
) -> None:
    if book_file is None:
        return
    if not book_file.exists():
        raise FileNotFoundError(f"book file not found: {book_file}")
    with chess.polyglot.open_reader(str(book_file)) as reader:
        for record in records:
            if book_depth >= 0 and record.ply > book_depth:
                continue
            board = chess.Board(record.fen_before)
            entries = [entry for entry in reader.find_all(board) if entry.weight >= minimum_weight]
            if not entries:
                continue
            entries.sort(key=lambda entry: (entry.weight, entry.learn), reverse=True)
            record.in_book = True
            record.book_move_count = len(entries)
            record.book_total_weight = sum(entry.weight for entry in entries)
            record.book_bestmove = entries[0].move.uci()
            for entry in entries:
                if entry.move.uci() == record.uci:
                    record.played_book_move = True
                    record.book_played_weight = entry.weight
                    break
            if not record.played_book_move:
                add_category(record, "book-deviation", 40)
            elif record.reference_avoid and record.book_bestmove != record.uci:
                add_category(record, "book-chose-weaker-line", 60)


def add_reference_bestmoves(
    events: list[MoveRecord],
    engine: UciTraceEngine,
    depth: int,
    movetime_ms: int,
    min_delta_cp: int,
    confirm_depth: int,
) -> None:
    if depth <= 0 and movetime_ms <= 0:
        return
    for event in events:
        best = engine.search(event.fen_before, depth, movetime_ms)
        if best.bestmove != "0000":
            event.reference_bestmove = best.bestmove
            event.reference_score_cp = best.score_cp
            event.reference_pv = format_pv(best.pv)
            event.reference_scan_depth = depth
            event.reference_scan_movetime_ms = movetime_ms
        if best.bestmove == "0000" or best.bestmove == event.uci or best.score_cp is None:
            continue

        board = chess.Board(event.fen_before)
        move = chess.Move.from_uci(event.uci)
        if move not in board.legal_moves:
            continue
        board.push(move)
        reply = engine.search(board.fen(), depth, movetime_ms)
        if reply.score_cp is None:
            continue
        event.reference_played_score_cp = -reply.score_cp
        event.reference_played_pv = format_pv(reply.pv)
        event.reference_delta_cp = best.score_cp - event.reference_played_score_cp
        event.reference_avoid = event.reference_delta_cp >= min_delta_cp
        if event.reference_avoid and confirm_depth > depth and movetime_ms <= 0:
            confirmed = engine.search(event.fen_before, confirm_depth)
            if confirmed.bestmove != "0000":
                event.reference_bestmove = confirmed.bestmove
                event.reference_score_cp = confirmed.score_cp
                event.reference_pv = format_pv(confirmed.pv)
            else:
                event.reference_avoid = False
                event.reference_played_score_cp = None
                event.reference_played_pv = ""
                event.reference_delta_cp = None
                continue
            if confirmed.bestmove == event.uci or confirmed.score_cp is None:
                event.reference_avoid = False
                event.reference_played_score_cp = None
                event.reference_played_pv = ""
                event.reference_delta_cp = None
                continue
            board = chess.Board(event.fen_before)
            move = chess.Move.from_uci(event.uci)
            if move not in board.legal_moves:
                event.reference_avoid = False
                event.reference_played_score_cp = None
                event.reference_played_pv = ""
                event.reference_delta_cp = None
                continue
            board.push(move)
            confirmed_reply = engine.search(board.fen(), confirm_depth)
            if confirmed_reply.score_cp is None:
                event.reference_avoid = False
                event.reference_played_score_cp = None
                event.reference_played_pv = ""
                event.reference_delta_cp = None
                continue
            event.reference_played_score_cp = -confirmed_reply.score_cp
            event.reference_played_pv = format_pv(confirmed_reply.pv)
            event.reference_delta_cp = confirmed.score_cp - event.reference_played_score_cp
            event.reference_avoid = event.reference_delta_cp >= min_delta_cp


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Analyze gauntlet PGNs for bad trades and evaluation swings.")
    parser.add_argument("--pgn-dir", required=True, type=Path, help="Gauntlet output directory or one PGN file.")
    parser.add_argument("--engine", default="./build-release/chess_uci", help="Engine command used for eval traces.")
    parser.add_argument(
        "--engine-option",
        action="append",
        default=[],
        metavar="NAME=VALUE",
        help="UCI option for --engine. Repeatable.",
    )
    parser.add_argument("--engine-name", required=True, help="Engine name as it appears in PGN White/Black headers.")
    parser.add_argument("--output-dir", type=Path, default=Path("runs/postmortems/latest"))
    parser.add_argument("--max-events", type=int, default=80)
    parser.add_argument("--swing-threshold", type=int, default=120)
    parser.add_argument("--trade-drop-threshold", type=int, default=50)
    parser.add_argument("--engine-depth", type=int, default=0, help="Optional depth for bestmove re-search on flagged events.")
    parser.add_argument("--reference-engine", default="", help="Optional UCI engine command used to annotate EPD bm moves.")
    parser.add_argument(
        "--reference-option",
        action="append",
        default=[],
        metavar="NAME=VALUE",
        help="UCI option for --reference-engine. Repeatable.",
    )
    parser.add_argument("--reference-depth", type=int, default=0, help="Reference-engine depth for EPD bm annotation.")
    parser.add_argument(
        "--reference-confirm-depth",
        type=int,
        default=0,
        help="Optional deeper reference depth used to confirm am labels before writing them.",
    )
    parser.add_argument("--reference-movetime-ms", type=int, default=0, help="Reference-engine movetime for EPD bm annotation.")
    parser.add_argument(
        "--reference-min-delta-cp",
        type=int,
        default=120,
        help="Only emit an EPD am move when the reference best move is at least this many cp better.",
    )
    parser.add_argument(
        "--reference-scan",
        choices=("all", "events", "none"),
        default="all",
        help=(
            "How much to score with the reference engine. all scans every analyzed move before event selection; "
            "events scores only preselected events; none disables reference scoring."
        ),
    )
    parser.add_argument("--book-file", type=Path, default=None, help="Optional Polyglot book used to annotate book exits.")
    parser.add_argument("--book-depth", type=int, default=255, help="Maximum ply for book annotations. Use -1 for unlimited.")
    parser.add_argument("--book-minimum-weight", type=int, default=1)
    parser.add_argument("--both-sides", action="store_true", help="Analyze both players instead of only --engine-name moves.")
    parser.add_argument("--protocol-timeout", type=float, default=10.0)
    return parser.parse_args()


def parse_option_assignments(raw_options: Iterable[str]) -> list[tuple[str, str]]:
    parsed: list[tuple[str, str]] = []
    for raw in raw_options:
        if "=" not in raw:
            raise ValueError(f"invalid UCI option {raw!r}; expected NAME=VALUE")
        name, value = raw.split("=", 1)
        name = name.strip()
        value = value.strip()
        if not name:
            raise ValueError(f"invalid UCI option {raw!r}; option name is empty")
        parsed.append((name, value))
    return parsed


def main() -> int:
    args = parse_args()
    try:
        engine_options = parse_option_assignments(args.engine_option)
        reference_options = parse_option_assignments(args.reference_option)
    except ValueError as error:
        print(str(error), file=sys.stderr)
        return 2
    paths = pgn_paths(args.pgn_dir)
    if not paths:
        print(f"no PGN files found in {args.pgn_dir}", file=sys.stderr)
        return 2
    if args.max_events <= 0:
        print("--max-events must be positive", file=sys.stderr)
        return 2

    args.output_dir.mkdir(parents=True, exist_ok=True)
    records: list[MoveRecord] = []
    with UciTraceEngine(args.engine, args.protocol_timeout, engine_options) as trace_engine:
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

    if args.reference_engine and args.reference_scan == "all":
        reference_depth = args.reference_depth
        if reference_depth <= 0 and args.reference_movetime_ms <= 0:
            reference_depth = 12
        reference_confirm_depth = max(0, args.reference_confirm_depth)
        with UciTraceEngine(args.reference_engine, args.protocol_timeout, reference_options) as reference_engine:
            add_reference_bestmoves(
                records,
                reference_engine,
                reference_depth,
                args.reference_movetime_ms,
                args.reference_min_delta_cp,
                reference_confirm_depth,
            )
        add_reference_diagnostic_categories(records, args.reference_min_delta_cp)

    annotate_book(records, args.book_file, args.book_minimum_weight, args.book_depth)

    events = select_events(records, args.max_events)

    with UciTraceEngine(args.engine, args.protocol_timeout, engine_options) as trace_engine:
        add_engine_bestmoves(events, trace_engine, args.engine_depth)

    if args.reference_engine and args.reference_scan == "events":
        reference_depth = args.reference_depth
        if reference_depth <= 0 and args.reference_movetime_ms <= 0:
            reference_depth = 12
        reference_confirm_depth = args.reference_confirm_depth
        if reference_confirm_depth <= 0 and reference_depth > 0 and args.reference_movetime_ms <= 0:
            reference_confirm_depth = reference_depth + 2
        with UciTraceEngine(args.reference_engine, args.protocol_timeout, reference_options) as reference_engine:
            add_reference_bestmoves(
                events,
                reference_engine,
                reference_depth,
                args.reference_movetime_ms,
                args.reference_min_delta_cp,
                reference_confirm_depth,
            )
        add_reference_diagnostic_categories(events, args.reference_min_delta_cp)

    if args.engine_depth > 0 and any(event.reference_bestmove for event in events):
        with UciTraceEngine(args.engine, args.protocol_timeout, engine_options) as trace_engine:
            add_engine_reference_scores(events, trace_engine, args.engine_depth)
    add_engine_diagnostic_categories(events)

    summary = summarize(records, events, args.engine_name)
    write_json(args.output_dir / "postmortem.json", summary, events)
    write_csv(args.output_dir / "moves.csv", records)
    write_csv(args.output_dir / "events.csv", events)
    write_game_summary_csv(args.output_dir / "games.csv", records)
    write_epd(args.output_dir / "positions.epd", events)
    write_markdown(args.output_dir / "report.md", summary, events)

    print(f"games_seen {summary['games_seen']}")
    print(f"engine_moves_analyzed {summary['engine_moves_analyzed']}")
    print(f"flagged_events {summary['flagged_events']}")
    print(f"report {args.output_dir / 'report.md'}")
    print(f"events {args.output_dir / 'events.csv'}")
    print(f"moves {args.output_dir / 'moves.csv'}")
    print(f"games {args.output_dir / 'games.csv'}")
    print(f"positions {args.output_dir / 'positions.epd'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
