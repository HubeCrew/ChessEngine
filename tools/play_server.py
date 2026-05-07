#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import mimetypes
import queue
import re
import subprocess
import threading
import time
from dataclasses import dataclass, field
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Callable
from urllib.parse import urlparse

import chess


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ENGINE = ROOT / "build-release" / "chess_uci"
FRONTEND_DIST = ROOT / "frontend" / "dist"

ENGINE_OPTIONS: dict[str, dict[str, Any]] = {
    "Hash": {"type": "spin", "default": 64, "min": 1, "max": 4096},
    "Threads": {"type": "spin", "default": 1, "min": 1, "max": 512},
    "MultiPV": {"type": "spin", "default": 1, "min": 1, "max": 256},
    "SyzygyPath": {"type": "string", "default": ""},
    "SyzygyProbeDepth": {"type": "spin", "default": 1, "min": 0, "max": 127},
    "OwnBook": {"type": "check", "default": True},
    "BookFile": {"type": "string", "default": ""},
    "BestBookMove": {"type": "check", "default": True},
    "BookDepth": {"type": "spin", "default": 255, "min": 0, "max": 255},
    "BookMinimumWeight": {"type": "spin", "default": 1, "min": 0, "max": 65535},
    "Move Overhead": {"type": "spin", "default": 20, "min": 0, "max": 5000},
    "Slow Mover": {"type": "spin", "default": 100, "min": 10, "max": 1000},
    "NullMovePruning": {"type": "check", "default": True},
    "SearchExtensions": {"type": "check", "default": True},
    "EvalType": {"type": "combo", "default": "classical", "vars": ["classical", "nnue", "hybrid"]},
    "NnueFile": {"type": "string", "default": ""},
}


def default_engine_options() -> dict[str, Any]:
    return {name: spec["default"] for name, spec in ENGINE_OPTIONS.items()}


def discover_files(pattern: str) -> list[str]:
    return [str(path.relative_to(ROOT)) for path in sorted(ROOT.glob(pattern)) if path.is_file()]


def json_dumps(payload: Any) -> bytes:
    return json.dumps(payload, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def parse_info_line(line: str) -> dict[str, Any]:
    tokens = line.split()
    parsed: dict[str, Any] = {"raw": line, "pv": []}
    index = 1
    while index < len(tokens):
        token = tokens[index]
        if token in {"depth", "multipv", "nodes", "qnodes", "nps", "time"} and index + 1 < len(tokens):
            try:
                parsed[token] = int(tokens[index + 1])
            except ValueError:
                parsed[token] = tokens[index + 1]
            index += 2
        elif token == "score" and index + 2 < len(tokens):
            parsed["scoreType"] = tokens[index + 1]
            try:
                parsed["score"] = int(tokens[index + 2])
            except ValueError:
                parsed["score"] = tokens[index + 2]
            index += 3
        elif token == "pv":
            parsed["pv"] = tokens[index + 1 :]
            break
        else:
            index += 1
    return parsed


@dataclass
class TimeControl:
    mode: str = "clock"
    base_ms: int = 300_000
    increment_ms: int = 2_000
    movestogo: int = 0
    movetime_ms: int = 1_000
    depth: int = 4


@dataclass
class MoveRecord:
    ply: int
    san: str
    uci: str
    fen: str
    by: str


@dataclass
class GameState:
    status: str = "setup"
    board: chess.Board = field(default_factory=chess.Board)
    human_color: chess.Color = chess.WHITE
    engine_color: chess.Color = chess.BLACK
    time_control: TimeControl = field(default_factory=TimeControl)
    engine_options: dict[str, Any] = field(default_factory=default_engine_options)
    moves: list[MoveRecord] = field(default_factory=list)
    white_clock_ms: int = 300_000
    black_clock_ms: int = 300_000
    running_clock_color: chess.Color | None = None
    clock_started_at: float | None = None
    last_search: dict[str, Any] = field(default_factory=dict)
    last_info_lines: list[dict[str, Any]] = field(default_factory=list)
    engine_searching: bool = False
    engine_ready: bool = False
    engine_error: str | None = None
    last_error: str | None = None
    clock_flag: chess.Color | None = None
    flipped: bool = False


class EngineProcess:
    def __init__(self, path: Path, on_line: Callable[[str], None]) -> None:
        self.path = path
        self.on_line = on_line
        self.process: subprocess.Popen[str] | None = None
        self.lines: queue.Queue[str] = queue.Queue()
        self.lock = threading.Lock()
        self.reader: threading.Thread | None = None

    def start(self) -> None:
        if self.process and self.process.poll() is None:
            return
        if not self.path.exists():
            raise FileNotFoundError(f"engine not found: {self.path}")
        self.process = subprocess.Popen(
            [str(self.path)],
            cwd=str(ROOT),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self.reader = threading.Thread(target=self._read_stdout, daemon=True)
        self.reader.start()
        self.send("uci")
        self.wait_for(lambda line: line == "uciok", timeout=5)

    def _read_stdout(self) -> None:
        assert self.process is not None and self.process.stdout is not None
        for raw in self.process.stdout:
            line = raw.strip()
            if not line:
                continue
            self.lines.put(line)
            self.on_line(line)

    def send(self, command: str) -> None:
        with self.lock:
            if not self.process or self.process.poll() is not None or not self.process.stdin:
                raise RuntimeError("engine process is not running")
            self.process.stdin.write(command + "\n")
            self.process.stdin.flush()

    def wait_for(self, predicate: Callable[[str], bool], timeout: float) -> str:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                line = self.lines.get(timeout=max(0.05, deadline - time.monotonic()))
            except queue.Empty:
                break
            if predicate(line):
                return line
        raise TimeoutError("engine did not respond in time")

    def ready(self) -> None:
        self.send("isready")
        self.wait_for(lambda line: line == "readyok", timeout=10)

    def close(self) -> None:
        if not self.process:
            return
        try:
            if self.process.poll() is None:
                self.send("quit")
                self.process.wait(timeout=2)
        except Exception:
            self.process.kill()
        finally:
            self.process = None


class PlayController:
    def __init__(self, engine_path: Path) -> None:
        self.engine_path = engine_path
        self.state = GameState()
        self.lock = threading.RLock()
        self.events: list[dict[str, Any]] = []
        self.event_condition = threading.Condition(self.lock)
        self.engine = EngineProcess(engine_path, self.handle_engine_line)
        self._timer = threading.Thread(target=self._clock_loop, daemon=True)
        self._timer.start()

    def emit(self, kind: str, payload: dict[str, Any] | None = None) -> None:
        event = {"id": len(self.events) + 1, "type": kind, "payload": payload or {}}
        self.events.append(event)
        self.events = self.events[-500:]
        self.event_condition.notify_all()

    def wait_events(self, last_id: int, timeout: float = 15.0) -> list[dict[str, Any]]:
        deadline = time.monotonic() + timeout
        with self.lock:
            while True:
                found = [event for event in self.events if event["id"] > last_id]
                if found or time.monotonic() >= deadline:
                    return found
                self.event_condition.wait(timeout=deadline - time.monotonic())

    def ensure_engine(self) -> None:
        self.engine.start()
        with self.lock:
            self.state.engine_ready = True
            self.state.engine_error = None

    def configure_engine(self, options: dict[str, Any]) -> None:
        for name, value in options.items():
            spec = ENGINE_OPTIONS.get(name)
            if not spec:
                continue
            if spec["type"] == "check":
                value_text = "true" if bool(value) else "false"
            else:
                value_text = str(value)
            self.engine.send(f"setoption name {name} value {value_text}")
        self.engine.ready()

    def new_game(self, payload: dict[str, Any]) -> dict[str, Any]:
        with self.lock:
            color_text = payload.get("humanColor", "white")
            human_color = chess.WHITE if color_text != "black" else chess.BLACK
            time_control = self._parse_time_control(payload.get("timeControl", {}))
            engine_options = self._parse_engine_options(payload.get("engineOptions", {}))
            self.state = GameState(
                status="playing",
                board=chess.Board(),
                human_color=human_color,
                engine_color=not human_color,
                time_control=time_control,
                engine_options=engine_options,
                white_clock_ms=time_control.base_ms,
                black_clock_ms=time_control.base_ms,
                running_clock_color=chess.WHITE,
                clock_started_at=time.monotonic(),
                engine_ready=self.state.engine_ready,
            )
        try:
            self.ensure_engine()
            self.engine.send("stop")
            self.engine.send("ucinewgame")
            self.configure_engine(engine_options)
        except Exception as error:
            with self.lock:
                self.state.status = "error"
                self.state.engine_error = str(error)
                self.state.last_error = str(error)
                self.emit("state", self.snapshot_locked())
            return self.snapshot()
        with self.lock:
            self.emit("state", self.snapshot_locked())
        if self.state.board.turn == self.state.engine_color:
            self.start_engine_search()
        return self.snapshot()

    def reset(self) -> dict[str, Any]:
        with self.lock:
            self._commit_clock_locked()
            self.state = GameState(engine_ready=self.state.engine_ready)
            self.emit("state", self.snapshot_locked())
        try:
            if self.engine.process and self.engine.process.poll() is None:
                self.engine.send("stop")
                self.engine.send("ucinewgame")
        except Exception:
            pass
        return self.snapshot()

    def stop(self) -> dict[str, Any]:
        try:
            self.engine.send("stop")
        except Exception as error:
            with self.lock:
                self.state.last_error = str(error)
        with self.lock:
            self.state.engine_searching = False
            self.emit("state", self.snapshot_locked())
        return self.snapshot()

    def apply_human_move(self, payload: dict[str, Any]) -> dict[str, Any]:
        uci = str(payload.get("uci", ""))
        with self.lock:
            if self.state.status != "playing":
                return self._error_locked("Start a game before moving.")
            if self.state.board.turn != self.state.human_color:
                return self._error_locked("The engine is thinking.")
            try:
                move = chess.Move.from_uci(uci)
            except ValueError:
                return self._error_locked("Move is not in UCI format.")
            if move not in self.state.board.legal_moves:
                return self._error_locked("Illegal move for the current position.")
            self._push_move_locked(move, "human")
            self._after_move_locked()
            self.emit("state", self.snapshot_locked())
        if self.state.status == "playing":
            self.start_engine_search()
        return self.snapshot()

    def start_engine_search(self) -> None:
        with self.lock:
            if self.state.board.turn != self.state.engine_color or self.state.status != "playing":
                return
            self._commit_clock_locked()
            self.state.running_clock_color = self.state.engine_color
            self.state.clock_started_at = time.monotonic()
            self.state.engine_searching = True
            self.state.last_search = {}
            self.state.last_info_lines = []
            fen = self.state.board.fen(en_passant="fen")
            command = self._go_command_locked()
        try:
            self.ensure_engine()
            self.engine.send(f"position fen {fen}")
            self.engine.send(command)
        except Exception as error:
            with self.lock:
                self.state.engine_searching = False
                self.state.engine_error = str(error)
                self.state.last_error = str(error)
                self.emit("state", self.snapshot_locked())

    def handle_engine_line(self, line: str) -> None:
        with self.lock:
            if line.startswith("info "):
                if " string " in line:
                    if "failed" in line or "error" in line:
                        self.state.last_error = line
                    self.emit("engine-info", {"raw": line})
                    return
                parsed = parse_info_line(line)
                if "score" in parsed:
                    self.state.last_search = parsed
                    self.state.last_info_lines = [parsed, *self.state.last_info_lines[:7]]
                self.emit("search", {"line": parsed, "state": self.snapshot_locked()})
                return
            if not line.startswith("bestmove "):
                return
            if self.state.status != "playing" or not self.state.engine_searching:
                return
            move_text = line.split()[1]
            self.state.engine_searching = False
            if move_text == "0000":
                self._after_move_locked()
                self.emit("state", self.snapshot_locked())
                return
            try:
                move = chess.Move.from_uci(move_text)
            except ValueError:
                self._error_locked(f"Engine returned malformed bestmove: {move_text}")
                return
            if move not in self.state.board.legal_moves:
                self._error_locked(f"Engine returned illegal bestmove: {move_text}")
                return
            self._push_move_locked(move, "engine")
            self._after_move_locked()
            self.emit("state", self.snapshot_locked())

    def _push_move_locked(self, move: chess.Move, by: str) -> None:
        self._commit_clock_locked()
        san = self.state.board.san(move)
        self.state.board.push(move)
        if self.state.time_control.mode == "clock":
            if by == "human":
                self._add_increment_locked(self.state.human_color)
            else:
                self._add_increment_locked(self.state.engine_color)
        self.state.moves.append(
            MoveRecord(
                ply=len(self.state.moves) + 1,
                san=san,
                uci=move.uci(),
                fen=self.state.board.fen(en_passant="fen"),
                by=by,
            )
        )
        self.state.running_clock_color = self.state.board.turn
        self.state.clock_started_at = time.monotonic()

    def _after_move_locked(self) -> None:
        if self.state.board.is_game_over(claim_draw=True):
            self.state.status = "gameover"
            self.state.running_clock_color = None
            self.state.clock_started_at = None
        elif self._clock_for_locked(chess.WHITE) <= 0:
            self.state.status = "gameover"
            self.state.clock_flag = chess.WHITE
            self.state.running_clock_color = None
            self.state.clock_started_at = None
        elif self._clock_for_locked(chess.BLACK) <= 0:
            self.state.status = "gameover"
            self.state.clock_flag = chess.BLACK
            self.state.running_clock_color = None
            self.state.clock_started_at = None

    def _commit_clock_locked(self) -> None:
        if self.state.time_control.mode != "clock":
            return
        if self.state.running_clock_color is None or self.state.clock_started_at is None:
            return
        elapsed_ms = int((time.monotonic() - self.state.clock_started_at) * 1000)
        if self.state.running_clock_color == chess.WHITE:
            self.state.white_clock_ms = max(0, self.state.white_clock_ms - elapsed_ms)
        else:
            self.state.black_clock_ms = max(0, self.state.black_clock_ms - elapsed_ms)
        self.state.clock_started_at = time.monotonic()

    def _add_increment_locked(self, color: chess.Color) -> None:
        increment = self.state.time_control.increment_ms
        if color == chess.WHITE:
            self.state.white_clock_ms += increment
        else:
            self.state.black_clock_ms += increment

    def _clock_for_locked(self, color: chess.Color) -> int:
        base = self.state.white_clock_ms if color == chess.WHITE else self.state.black_clock_ms
        if self.state.time_control.mode != "clock":
            return base
        if self.state.running_clock_color != color or self.state.clock_started_at is None:
            return base
        elapsed_ms = int((time.monotonic() - self.state.clock_started_at) * 1000)
        return max(0, base - elapsed_ms)

    def _go_command_locked(self) -> str:
        control = self.state.time_control
        if control.mode == "depth":
            return f"go depth {max(1, control.depth)}"
        if control.mode == "movetime":
            return f"go movetime {max(1, control.movetime_ms)}"
        parts = [
            "go",
            "wtime",
            str(self._clock_for_locked(chess.WHITE)),
            "btime",
            str(self._clock_for_locked(chess.BLACK)),
            "winc",
            str(max(0, control.increment_ms)),
            "binc",
            str(max(0, control.increment_ms)),
        ]
        if control.movestogo > 0:
            parts.extend(["movestogo", str(control.movestogo)])
        return " ".join(parts)

    def _parse_time_control(self, raw: dict[str, Any]) -> TimeControl:
        mode = str(raw.get("mode", "clock"))
        if mode not in {"clock", "movetime", "depth"}:
            mode = "clock"
        return TimeControl(
            mode=mode,
            base_ms=max(0, int(raw.get("baseMs", 300_000))),
            increment_ms=max(0, int(raw.get("incrementMs", 2_000))),
            movestogo=max(0, int(raw.get("movesToGo", 0))),
            movetime_ms=max(1, int(raw.get("moveTimeMs", 1_000))),
            depth=max(1, int(raw.get("depth", 4))),
        )

    def _parse_engine_options(self, raw: dict[str, Any]) -> dict[str, Any]:
        parsed = default_engine_options()
        parsed.update(raw)
        for name, spec in ENGINE_OPTIONS.items():
            value = parsed[name]
            if spec["type"] == "spin":
                parsed[name] = min(spec["max"], max(spec["min"], int(value)))
            elif spec["type"] == "check":
                parsed[name] = bool(value)
            elif spec["type"] == "combo":
                parsed[name] = value if value in spec["vars"] else spec["default"]
            else:
                parsed[name] = str(value)
        return parsed

    def _error_locked(self, message: str) -> dict[str, Any]:
        self.state.last_error = message
        self.emit("error", {"message": message, "state": self.snapshot_locked()})
        return self.snapshot_locked()

    def _clock_loop(self) -> None:
        while True:
            time.sleep(1)
            with self.lock:
                if self.state.status == "playing" and self.state.time_control.mode == "clock":
                    if self._clock_for_locked(chess.WHITE) <= 0:
                        self.state.status = "gameover"
                        self.state.clock_flag = chess.WHITE
                        self.state.running_clock_color = None
                        self.state.clock_started_at = None
                    elif self._clock_for_locked(chess.BLACK) <= 0:
                        self.state.status = "gameover"
                        self.state.clock_flag = chess.BLACK
                        self.state.running_clock_color = None
                        self.state.clock_started_at = None
                    self.emit("tick", self.snapshot_locked())

    def snapshot(self) -> dict[str, Any]:
        with self.lock:
            return self.snapshot_locked()

    def snapshot_locked(self) -> dict[str, Any]:
        board = self.state.board
        legal_moves = [move.uci() for move in board.legal_moves] if self.state.status == "playing" else []
        result = board.result(claim_draw=True) if board.is_game_over(claim_draw=True) else "*"
        return {
            "status": self.state.status,
            "enginePath": str(self.engine_path.relative_to(ROOT)) if self.engine_path.is_relative_to(ROOT) else str(self.engine_path),
            "engineReady": self.state.engine_ready,
            "engineSearching": self.state.engine_searching,
            "engineError": self.state.engine_error,
            "lastError": self.state.last_error,
            "fen": board.fen(en_passant="fen"),
            "turn": "white" if board.turn == chess.WHITE else "black",
            "humanColor": "white" if self.state.human_color == chess.WHITE else "black",
            "engineColor": "white" if self.state.engine_color == chess.WHITE else "black",
            "legalMoves": legal_moves,
            "moves": [record.__dict__ for record in self.state.moves],
            "result": result,
            "outcome": self._outcome_locked(),
            "whiteClockMs": self._clock_for_locked(chess.WHITE),
            "blackClockMs": self._clock_for_locked(chess.BLACK),
            "timeControl": self.state.time_control.__dict__,
            "engineOptions": self.state.engine_options,
            "engineOptionSpecs": ENGINE_OPTIONS,
            "lastSearch": self.state.last_search,
            "lastInfoLines": self.state.last_info_lines,
            "suggestions": {
                "nnueFiles": discover_files("runs/**/*.nnue"),
                "bookFiles": discover_files("runs/**/*.bin"),
                "syzygyDirs": [str(path.relative_to(ROOT)) for path in sorted((ROOT / "runs").glob("**/syzygy*")) if path.is_dir()],
            },
        }

    def _outcome_locked(self) -> str | None:
        if self.state.clock_flag is not None:
            winner = "black" if self.state.clock_flag == chess.WHITE else "white"
            return f"{winner} on time"
        if not self.state.board.is_game_over(claim_draw=True):
            return None
        outcome = self.state.board.outcome(claim_draw=True)
        if outcome is None:
            return None
        if outcome.winner is None:
            return outcome.termination.name.lower().replace("_", " ")
        winner = "white" if outcome.winner == chess.WHITE else "black"
        return f"{winner} by {outcome.termination.name.lower().replace('_', ' ')}"


class PlayRequestHandler(BaseHTTPRequestHandler):
    controller: PlayController
    static_root: Path

    def log_message(self, fmt: str, *args: Any) -> None:
        print(f"{self.address_string()} - {fmt % args}")

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/api/state":
            self.send_json(self.controller.snapshot())
            return
        if parsed.path == "/api/events":
            self.stream_events()
            return
        self.serve_static(parsed.path)

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        payload = self.read_json()
        if parsed.path == "/api/new-game":
            self.send_json(self.controller.new_game(payload))
        elif parsed.path == "/api/move":
            self.send_json(self.controller.apply_human_move(payload))
        elif parsed.path == "/api/reset":
            self.send_json(self.controller.reset())
        elif parsed.path == "/api/stop":
            self.send_json(self.controller.stop())
        else:
            self.send_error(HTTPStatus.NOT_FOUND)

    def read_json(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", "0"))
        if length == 0:
            return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def send_json(self, payload: Any, status: HTTPStatus = HTTPStatus.OK) -> None:
        body = json_dumps(payload)
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def stream_events(self) -> None:
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Connection", "keep-alive")
        self.end_headers()
        last_id = 0
        try:
            self.wfile.write(b"event: state\n")
            self.wfile.write(f"data: {json.dumps(self.controller.snapshot())}\n\n".encode("utf-8"))
            self.wfile.flush()
            while True:
                for event in self.controller.wait_events(last_id):
                    last_id = event["id"]
                    self.wfile.write(f"id: {event['id']}\n".encode("utf-8"))
                    self.wfile.write(f"event: {event['type']}\n".encode("utf-8"))
                    self.wfile.write(f"data: {json.dumps(event['payload'])}\n\n".encode("utf-8"))
                    self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            return

    def serve_static(self, path: str) -> None:
        if not self.static_root.exists():
            self.send_error(HTTPStatus.NOT_FOUND, "frontend has not been built; run npm run build in frontend/")
            return
        target = self.static_root / "index.html" if path == "/" else self.static_root / path.lstrip("/")
        target = target.resolve()
        if not str(target).startswith(str(self.static_root.resolve())) or not target.exists() or target.is_dir():
            target = self.static_root / "index.html"
        body = target.read_bytes()
        content_type = mimetypes.guess_type(target.name)[0] or "application/octet-stream"
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def main() -> int:
    parser = argparse.ArgumentParser(description="Serve the browser chess playing surface.")
    parser.add_argument("--engine", type=Path, default=DEFAULT_ENGINE, help="Path to the UCI engine binary.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8787)
    args = parser.parse_args()

    controller = PlayController(args.engine.resolve())
    PlayRequestHandler.controller = controller
    PlayRequestHandler.static_root = FRONTEND_DIST
    server = ThreadingHTTPServer((args.host, args.port), PlayRequestHandler)
    print(f"play server: http://{args.host}:{args.port}")
    print(f"engine: {args.engine}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        controller.engine.close()
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
