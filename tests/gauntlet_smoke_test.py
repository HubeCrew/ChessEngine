import csv
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
    return require_invalid_move_detection(gauntlet, chess_uci, referee)


if __name__ == "__main__":
    raise SystemExit(main())
