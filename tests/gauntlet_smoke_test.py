import csv
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def run_command(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=30,
    )


def write_invalid_engine(path: Path) -> None:
    path.write_text(
        "\n".join(
            [
                "import sys",
                "log_path = sys.argv[1] if len(sys.argv) > 1 else None",
                "for line in sys.stdin:",
                "    command = line.strip()",
                "    if command == 'uci':",
                "        print('id name InvalidEngine', flush=True)",
                "        print('uciok', flush=True)",
                "    elif command.startswith('setoption ') and log_path is not None:",
                "        with open(log_path, 'a', encoding='utf-8') as handle:",
                "            handle.write(command + '\\n')",
                "    elif command == 'isready':",
                "        print('readyok', flush=True)",
                "    elif command.startswith('go '):",
                "        print('bestmove a1a1', flush=True)",
                "    elif command == 'quit':",
                "        break",
                "",
            ]
        ),
        encoding="utf-8",
    )


def write_logging_engine(path: Path) -> None:
    path.write_text(
        "\n".join(
            [
                "import sys",
                "log_path = sys.argv[1]",
                "for line in sys.stdin:",
                "    command = line.strip()",
                "    if command == 'uci':",
                "        print('id name LoggingEngine', flush=True)",
                "        print('uciok', flush=True)",
                "    elif command == 'isready':",
                "        print('readyok', flush=True)",
                "    elif command.startswith('go '):",
                "        with open(log_path, 'a', encoding='utf-8') as handle:",
                "            handle.write(command + '\\n')",
                "        print('bestmove e2e4', flush=True)",
                "    elif command == 'quit':",
                "        break",
                "",
            ]
        ),
        encoding="utf-8",
    )


def write_silent_engine(path: Path) -> None:
    path.write_text(
        "\n".join(
            [
                "import sys",
                "for line in sys.stdin:",
                "    command = line.strip()",
                "    if command == 'uci':",
                "        print('id name SilentEngine', flush=True)",
                "        print('uciok', flush=True)",
                "    elif command == 'isready':",
                "        print('readyok', flush=True)",
                "    elif command == 'quit':",
                "        break",
                "",
            ]
        ),
        encoding="utf-8",
    )


def require_successful_self_play(gauntlet: Path, chess_uci: Path, referee: Path) -> int:
    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir) / "self-play"
        completed = run_command(
            [
                sys.executable,
                str(gauntlet),
                "--engine-a",
                str(chess_uci),
                "--engine-b",
                str(chess_uci),
                "--name-a",
                "current-a",
                "--name-b",
                "current-b",
                "--referee",
                str(referee),
                "--games",
                "2",
                "--movetime",
                "1",
                "--max-plies",
                "8",
                "--hash",
                "1",
                "--output-dir",
                str(output_dir),
                "--csv",
            ]
        )
        if completed.returncode != 0:
            print(completed.stdout, file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return completed.returncode
        if "games 2" not in completed.stdout or "confidence rough" not in completed.stdout:
            print(completed.stdout, file=sys.stderr)
            return 1

        csv_path = output_dir / "results.csv"
        rows = list(csv.DictReader(csv_path.read_text(encoding="utf-8").splitlines()))
        if len(rows) != 2:
            print(f"expected 2 CSV rows, got {len(rows)}", file=sys.stderr)
            return 1
        if not (output_dir / "game-0001.pgn").exists():
            print("missing PGN output", file=sys.stderr)
            return 1
        live_state = json.loads((output_dir / "live-state.json").read_text(encoding="utf-8"))
        if live_state["schema"] != 1 or live_state["total_games"] != 2:
            print(f"unexpected live state header: {live_state}", file=sys.stderr)
            return 1
        if live_state["summary"]["games"] != 2 or len(live_state["results"]) != 2:
            print(f"unexpected live state results: {live_state}", file=sys.stderr)
            return 1
        if live_state["current"]["game"] != 2 or not isinstance(live_state["current"]["moves"], list):
            print(f"unexpected current live game: {live_state}", file=sys.stderr)
            return 1

    return 0


def require_engine_failure_details(gauntlet: Path, chess_uci: Path, referee: Path) -> int:
    with tempfile.TemporaryDirectory() as temp_dir:
        dummy = Path(temp_dir) / "silent_engine.py"
        output_dir = Path(temp_dir) / "silent"
        write_silent_engine(dummy)
        completed = run_command(
            [
                sys.executable,
                str(gauntlet),
                "--engine-a",
                f"{sys.executable} {dummy}",
                "--engine-b",
                str(chess_uci),
                "--name-a",
                "silent",
                "--name-b",
                "current",
                "--referee",
                str(referee),
                "--games",
                "1",
                "--movetime",
                "1",
                "--max-plies",
                "2",
                "--hash",
                "1",
                "--protocol-timeout",
                "0.1",
                "--output-dir",
                str(output_dir),
                "--no-pgn",
            ]
        )
        if completed.returncode == 0:
            print("expected gauntlet to fail for silent engine", file=sys.stderr)
            print(completed.stdout, file=sys.stderr)
            return 1
        if "engine_failure:" not in completed.stdout or "protocol_timeout" not in completed.stdout:
            print("expected detailed engine failure reason", file=sys.stderr)
            print(completed.stdout, file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return 1

    return 0


def require_clock_self_play(gauntlet: Path, chess_uci: Path, referee: Path) -> int:
    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir) / "clock-play"
        completed = run_command(
            [
                sys.executable,
                str(gauntlet),
                "--engine-a",
                str(chess_uci),
                "--engine-b",
                str(chess_uci),
                "--name-a",
                "clock-a",
                "--name-b",
                "clock-b",
                "--referee",
                str(referee),
                "--games",
                "2",
                "--time",
                "1000",
                "--increment",
                "20",
                "--moves-to-go",
                "20",
                "--max-plies",
                "8",
                "--hash",
                "1",
                "--output-dir",
                str(output_dir),
                "--csv",
                "--no-pgn",
            ]
        )
        if completed.returncode != 0:
            print(completed.stdout, file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return completed.returncode
        if "games 2" not in completed.stdout:
            print(completed.stdout, file=sys.stderr)
            return 1

        rows = list(csv.DictReader((output_dir / "results.csv").read_text(encoding="utf-8").splitlines()))
        if len(rows) != 2:
            print(f"expected 2 clock CSV rows, got {len(rows)}", file=sys.stderr)
            return 1

    return 0


def require_clock_go_command(gauntlet: Path, chess_uci: Path, referee: Path) -> int:
    with tempfile.TemporaryDirectory() as temp_dir:
        dummy = Path(temp_dir) / "logging_engine.py"
        go_log = Path(temp_dir) / "go.txt"
        output_dir = Path(temp_dir) / "clock-command"
        write_logging_engine(dummy)
        completed = run_command(
            [
                sys.executable,
                str(gauntlet),
                "--engine-a",
                f"{sys.executable} {dummy} {go_log}",
                "--engine-b",
                str(chess_uci),
                "--name-a",
                "logging",
                "--name-b",
                "current",
                "--referee",
                str(referee),
                "--games",
                "1",
                "--time",
                "1000",
                "--increment",
                "20",
                "--moves-to-go",
                "15",
                "--max-plies",
                "1",
                "--hash",
                "1",
                "--output-dir",
                str(output_dir),
                "--no-pgn",
            ]
        )
        if completed.returncode != 0:
            print(completed.stdout, file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return completed.returncode
        logged = go_log.read_text(encoding="utf-8")
        required = ["wtime", "btime", "winc 20", "binc 20", "movestogo 15"]
        for fragment in required:
            if fragment not in logged:
                print(f"missing {fragment!r} from logged go command: {logged!r}", file=sys.stderr)
                return 1

    return 0


def require_invalid_move_detection(gauntlet: Path, chess_uci: Path, referee: Path) -> int:
    with tempfile.TemporaryDirectory() as temp_dir:
        dummy = Path(temp_dir) / "invalid_engine.py"
        option_log = Path(temp_dir) / "options.txt"
        output_dir = Path(temp_dir) / "invalid"
        write_invalid_engine(dummy)
        completed = run_command(
            [
                sys.executable,
                str(gauntlet),
                "--engine-a",
                str(chess_uci),
                "--engine-b",
                f"{sys.executable} {dummy} {option_log}",
                "--name-a",
                "current",
                "--name-b",
                "invalid",
                "--referee",
                str(referee),
                "--games",
                "1",
                "--movetime",
                "1",
                "--max-plies",
                "4",
                "--hash",
                "1",
                "--option-b",
                "UCI_LimitStrength=true",
                "--option-b",
                "UCI_Elo=1320",
                "--output-dir",
                str(output_dir),
                "--no-pgn",
            ]
        )
        if completed.returncode == 0:
            print("expected gauntlet to fail for invalid engine", file=sys.stderr)
            print(completed.stdout, file=sys.stderr)
            return 1
        if "illegal_move" not in completed.stdout:
            print(completed.stdout, file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return 1
        options = option_log.read_text(encoding="utf-8")
        if "setoption name UCI_LimitStrength value true" not in options:
            print(f"missing UCI_LimitStrength option in {options!r}", file=sys.stderr)
            return 1
        if "setoption name UCI_Elo value 1320" not in options:
            print(f"missing UCI_Elo option in {options!r}", file=sys.stderr)
            return 1

    return 0


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: gauntlet_smoke_test.py <gauntlet.py> <chess_uci> <chess_referee>", file=sys.stderr)
        return 2

    gauntlet = Path(sys.argv[1])
    chess_uci = Path(sys.argv[2])
    referee = Path(sys.argv[3])

    result = require_successful_self_play(gauntlet, chess_uci, referee)
    if result != 0:
        return result
    result = require_engine_failure_details(gauntlet, chess_uci, referee)
    if result != 0:
        return result
    result = require_clock_self_play(gauntlet, chess_uci, referee)
    if result != 0:
        return result
    result = require_clock_go_command(gauntlet, chess_uci, referee)
    if result != 0:
        return result
    return require_invalid_move_detection(gauntlet, chess_uci, referee)


if __name__ == "__main__":
    raise SystemExit(main())
