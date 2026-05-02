#!/usr/bin/env python3
import argparse
import csv
import math
import queue
import shlex
import subprocess
import sys
import threading
import time
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable


OPENING_FENS = [
    ("startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"),
    ("italian", "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3"),
    ("queens_gambit", "rnbqkbnr/ppp1pppp/8/3p4/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2"),
    ("sicilian", "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2"),
    ("french", "rnbqkbnr/ppp2ppp/4p3/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3"),
    ("caro_kann", "rnbqkbnr/pp1ppppp/2p5/8/3PP3/8/PPP2PPP/RNBQKBNR b KQkq d3 0 2"),
    ("kings_indian", "rnbqk2r/ppppppbp/5np1/8/2PPP3/2N2N2/PP3PPP/R1BQKB1R b KQkq - 4 4"),
    ("english", "rnbqkbnr/pppppppp/8/8/2P5/8/PP1PPPPP/RNBQKBNR b KQkq c3 0 1"),
    ("ruy_lopez", "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3"),
    ("slav", "rnbqkbnr/pp2pppp/2p5/3p4/2PP4/8/PP2PPPP/RNBQKBNR w KQkq - 0 3"),
]


@dataclass
class EngineConfig:
    command: list[str]
    name: str
    hash_mb: int
    options: list[tuple[str, str]]


@dataclass
class GameResult:
    game: int
    opening: str
    white: str
    black: str
    result: str
    reason: str
    plies: int
    moves: list[str]
    clean: bool


class UciError(RuntimeError):
    pass


class UciEngine:
    def __init__(self, config: EngineConfig, protocol_timeout: float) -> None:
        self.config = config
        self.protocol_timeout = protocol_timeout
        self.process: subprocess.Popen[str] | None = None
        self.lines: queue.Queue[str | None] = queue.Queue()
        self.reader: threading.Thread | None = None

    def start(self) -> None:
        self.process = subprocess.Popen(
            self.config.command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )
        if self.process.stdout is None or self.process.stdin is None:
            raise UciError(f"{self.config.name}: failed to open engine pipes")
        self.reader = threading.Thread(target=self._read_stdout, daemon=True)
        self.reader.start()
        self._send("uci")
        self._read_until("uciok", self.protocol_timeout)
        if self.config.hash_mb > 0:
            self._send(f"setoption name Hash value {self.config.hash_mb}")
        for name, value in self.config.options:
            self._send(f"setoption name {name} value {value}")
        self.wait_ready()

    def new_game(self) -> None:
        self._send("ucinewgame")
        self.wait_ready()

    def wait_ready(self) -> None:
        self._send("isready")
        self._read_until("readyok", self.protocol_timeout)

    def bestmove(self, fen: str, moves: list[str], movetime_ms: int) -> str:
        command = f"position fen {fen}"
        if moves:
            command += " moves " + " ".join(moves)
        self._send(command)
        self._send(f"go movetime {movetime_ms}")
        deadline = time.monotonic() + max(self.protocol_timeout, movetime_ms / 1000.0 + self.protocol_timeout)
        while True:
            line = self._readline(deadline)
            if line.startswith("bestmove "):
                parts = line.split()
                if len(parts) < 2:
                    raise UciError(f"{self.config.name}: malformed bestmove line: {line}")
                return parts[1]

    def close(self) -> None:
        if self.process is None:
            return
        try:
            if self.process.poll() is None:
                self._send("quit")
                self.process.wait(timeout=1)
        except Exception:
            self.process.kill()
        finally:
            self.process = None
            self.reader = None

    def _send(self, command: str) -> None:
        if self.process is None or self.process.stdin is None or self.process.poll() is not None:
            raise UciError(f"{self.config.name}: engine is not running")
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def _read_until(self, expected: str, timeout: float) -> None:
        deadline = time.monotonic() + timeout
        while True:
            line = self._readline(deadline)
            if line == expected:
                return

    def _readline(self, deadline: float) -> str:
        if self.process is None:
            raise UciError(f"{self.config.name}: engine is not running")
        if self.process.poll() is not None:
            raise UciError(f"{self.config.name}: engine exited with code {self.process.returncode}")

        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise UciError(f"{self.config.name}: protocol timeout")
        try:
            line = self.lines.get(timeout=remaining)
        except queue.Empty:
            raise UciError(f"{self.config.name}: protocol timeout")
        if line is None:
            raise UciError(f"{self.config.name}: engine closed stdout")
        return line

    def _read_stdout(self) -> None:
        assert self.process is not None
        assert self.process.stdout is not None
        for line in self.process.stdout:
            self.lines.put(line.strip())
        self.lines.put(None)


def parse_command(value: str) -> list[str]:
    return shlex.split(value)


def parse_engine_option(value: str) -> tuple[str, str]:
    if "=" not in value:
        raise argparse.ArgumentTypeError("engine options must use NAME=VALUE syntax")
    name, option_value = value.split("=", 1)
    name = name.strip()
    option_value = option_value.strip()
    if not name:
        raise argparse.ArgumentTypeError("engine option name cannot be empty")
    return name, option_value


def run_referee(referee: Path, fen: str, moves: list[str], timeout: float) -> dict[str, str]:
    command = [str(referee), "--fen", fen, "--moves", *moves]
    completed = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=timeout,
    )
    fields: dict[str, str] = {}
    for line in completed.stdout.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            fields[key] = value
    if "ok" not in fields:
        raise RuntimeError(f"referee failed: {completed.stderr or completed.stdout}")
    return fields


def result_from_outcome(outcome: str) -> str:
    if outcome == "white_win":
        return "1-0"
    if outcome == "black_win":
        return "0-1"
    if outcome == "draw":
        return "1/2-1/2"
    return "*"


def forfeit_result(side: str) -> str:
    return "0-1" if side == "white" else "1-0"


def forfeit_outcome(side: str) -> str:
    return "black_win" if side == "white" else "white_win"


def write_pgn(path: Path, game: GameResult, start_fen: str) -> None:
    headers = {
        "Event": "ChessEngine Gauntlet",
        "Site": "local",
        "Date": datetime.now().strftime("%Y.%m.%d"),
        "Round": str(game.game),
        "White": game.white,
        "Black": game.black,
        "Result": game.result,
        "Opening": game.opening,
        "SetUp": "1",
        "FEN": start_fen,
        "Termination": game.reason,
    }
    lines = [f'[{key} "{value}"]' for key, value in headers.items()]
    lines.append("")

    move_text: list[str] = []
    for index, move in enumerate(game.moves):
        if index % 2 == 0:
            move_text.append(f"{index // 2 + 1}.")
        move_text.append(move)
    move_text.append(game.result)
    lines.append(" ".join(move_text))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def score_for_engine(results: Iterable[GameResult], engine_name: str) -> float:
    score = 0.0
    for result in results:
        if result.result == "1/2-1/2":
            score += 0.5
        elif result.result == "1-0" and result.white == engine_name:
            score += 1.0
        elif result.result == "0-1" and result.black == engine_name:
            score += 1.0
    return score


def elo_diff(score: float, games: int) -> float | None:
    if games <= 0:
        return None
    percentage = min(0.99, max(0.01, score / games))
    return -400.0 * math.log10(1.0 / percentage - 1.0)


def play_game(
    game_number: int,
    opening_name: str,
    start_fen: str,
    white: UciEngine,
    black: UciEngine,
    referee: Path,
    movetime_ms: int,
    max_plies: int,
    referee_timeout: float,
) -> GameResult:
    moves: list[str] = []
    status = run_referee(referee, start_fen, moves, referee_timeout)

    while status["outcome"] == "ongoing" and len(moves) < max_plies:
        side = status["side"]
        engine = white if side == "white" else black
        try:
            move = engine.bestmove(start_fen, moves, movetime_ms)
        except UciError:
            return GameResult(
                game_number,
                opening_name,
                white.config.name,
                black.config.name,
                forfeit_result(side),
                f"{forfeit_outcome(side)}:engine_failure",
                len(moves),
                moves,
                False,
            )

        candidate_moves = [*moves, move]
        status = run_referee(referee, start_fen, candidate_moves, referee_timeout)
        if status["ok"] != "true":
            return GameResult(
                game_number,
                opening_name,
                white.config.name,
                black.config.name,
                result_from_outcome(status["outcome"]),
                f"{status['outcome']}:illegal_move:{move}",
                len(moves),
                moves,
                False,
            )
        moves.append(move)

    if status["outcome"] == "ongoing":
        return GameResult(
            game_number,
            opening_name,
            white.config.name,
            black.config.name,
            "1/2-1/2",
            "draw:max_plies",
            len(moves),
            moves,
            True,
        )

    return GameResult(
        game_number,
        opening_name,
        white.config.name,
        black.config.name,
        result_from_outcome(status["outcome"]),
        f"{status['outcome']}:{status['reason']}",
        len(moves),
        moves,
        True,
    )


def write_csv(path: Path, results: list[GameResult]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=["game", "opening", "white", "black", "result", "reason", "plies", "clean"],
        )
        writer.writeheader()
        for result in results:
            writer.writerow(
                {
                    "game": result.game,
                    "opening": result.opening,
                    "white": result.white,
                    "black": result.black,
                    "result": result.result,
                    "reason": result.reason,
                    "plies": result.plies,
                    "clean": str(result.clean).lower(),
                }
            )


def print_summary(results: list[GameResult], engine_a: str, engine_b: str) -> None:
    games = len(results)
    score_a = score_for_engine(results, engine_a)
    score_b = games - score_a
    clean_games = sum(1 for result in results if result.clean)
    wins_a = sum(
        1
        for result in results
        if (result.result == "1-0" and result.white == engine_a)
        or (result.result == "0-1" and result.black == engine_a)
    )
    wins_b = sum(
        1
        for result in results
        if (result.result == "1-0" and result.white == engine_b)
        or (result.result == "0-1" and result.black == engine_b)
    )
    draws = sum(1 for result in results if result.result == "1/2-1/2")
    elo = elo_diff(score_a, games)

    print(f"games {games}")
    print(f"clean_games {clean_games}")
    print(f"{engine_a} score {score_a:.1f}/{games} wins {wins_a}")
    print(f"{engine_b} score {score_b:.1f}/{games} wins {wins_b}")
    print(f"draws {draws}")
    if elo is not None:
        print(f"elo_diff {engine_a} {elo:+.1f} vs {engine_b}")
    if games < 200:
        print("confidence rough")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a UCI engine gauntlet.")
    parser.add_argument("--engine-a", default="./build/chess_uci", help="command for engine A")
    parser.add_argument("--engine-b", default="./build/chess_uci", help="command for engine B")
    parser.add_argument("--name-a", default="engine_a")
    parser.add_argument("--name-b", default="engine_b")
    parser.add_argument("--referee", default="./build/chess_referee")
    parser.add_argument("--games", type=int, default=20)
    parser.add_argument("--movetime", type=int, default=100, help="milliseconds per move")
    parser.add_argument("--hash", type=int, default=16, help="Hash option in MB for both engines")
    parser.add_argument(
        "--option-a",
        action="append",
        default=[],
        type=parse_engine_option,
        metavar="NAME=VALUE",
        help="extra UCI option for engine A; may be repeated",
    )
    parser.add_argument(
        "--option-b",
        action="append",
        default=[],
        type=parse_engine_option,
        metavar="NAME=VALUE",
        help="extra UCI option for engine B; may be repeated",
    )
    parser.add_argument("--max-plies", type=int, default=300)
    parser.add_argument("--output-dir", default="gauntlet-results")
    parser.add_argument("--csv", action="store_true", help="write results.csv")
    parser.add_argument("--no-pgn", action="store_true", help="skip PGN output")
    parser.add_argument("--protocol-timeout", type=float, default=5.0)
    args = parser.parse_args()

    if args.games <= 0:
        print("--games must be positive", file=sys.stderr)
        return 2
    if args.movetime <= 0:
        print("--movetime must be positive", file=sys.stderr)
        return 2

    referee = Path(args.referee)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    config_a = EngineConfig(parse_command(args.engine_a), args.name_a, args.hash, args.option_a)
    config_b = EngineConfig(parse_command(args.engine_b), args.name_b, args.hash, args.option_b)
    engine_a = UciEngine(config_a, args.protocol_timeout)
    engine_b = UciEngine(config_b, args.protocol_timeout)
    results: list[GameResult] = []

    try:
        engine_a.start()
        engine_b.start()
        for game_index in range(args.games):
            opening_name, fen = OPENING_FENS[(game_index // 2) % len(OPENING_FENS)]
            if game_index % 2 == 0:
                white, black = engine_a, engine_b
            else:
                white, black = engine_b, engine_a

            white.new_game()
            black.new_game()
            result = play_game(
                game_index + 1,
                opening_name,
                fen,
                white,
                black,
                referee,
                args.movetime,
                args.max_plies,
                args.protocol_timeout,
            )
            results.append(result)
            print(
                f"game {result.game}: {result.white} vs {result.black} "
                f"{result.result} {result.reason} plies={result.plies}",
                flush=True,
            )
            if not args.no_pgn:
                write_pgn(output_dir / f"game-{result.game:04d}.pgn", result, fen)

        if args.csv:
            write_csv(output_dir / "results.csv", results)
        print_summary(results, args.name_a, args.name_b)
    finally:
        engine_a.close()
        engine_b.close()

    return 0 if all(result.clean for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
