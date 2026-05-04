import csv
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: postmortem_smoke_test.py <gauntlet_postmortem.py> <chess_uci>", file=sys.stderr)
        return 2

    try:
        import chess  # noqa: F401
    except ImportError:
        print("python-chess not installed; skipping postmortem smoke", file=sys.stderr)
        return 0

    tool = Path(sys.argv[1])
    engine = Path(sys.argv[2])
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        pgn_dir = root / "gauntlet"
        output_dir = root / "postmortem"
        pgn_dir.mkdir()
        (pgn_dir / "game-0001.pgn").write_text(
            "\n".join(
                [
                    '[Event "Smoke"]',
                    '[Site "local"]',
                    '[Date "2026.05.03"]',
                    '[Round "1"]',
                    '[White "current"]',
                    '[Black "opponent"]',
                    '[Result "0-1"]',
                    '[Opening "smoke"]',
                    '[SetUp "1"]',
                    '[FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"]',
                    '[Termination "black_win:smoke"]',
                    "",
                    "1. e2e4 e7e5 2. d1h5 b8c6 3. h5f7 e8f7 0-1",
                    "",
                ]
            ),
            encoding="utf-8",
        )
        completed = subprocess.run(
            [
                sys.executable,
                str(tool),
                "--pgn-dir",
                str(pgn_dir),
                "--engine",
                str(engine),
                "--engine-name",
                "current",
                "--output-dir",
                str(output_dir),
                "--max-events",
                "8",
                "--engine-depth",
                "1",
                "--reference-engine",
                str(engine),
                "--reference-depth",
                "1",
            ],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=20,
        )
        if completed.returncode != 0:
            print(completed.stdout, file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return completed.returncode

        report = (output_dir / "report.md").read_text(encoding="utf-8")
        payload = json.loads((output_dir / "postmortem.json").read_text(encoding="utf-8"))
        rows = list(csv.DictReader((output_dir / "events.csv").read_text(encoding="utf-8").splitlines()))
        positions = (output_dir / "positions.epd").read_text(encoding="utf-8")

        if payload["summary"]["games_seen"] != 1 or payload["summary"]["flagged_events"] <= 0:
            print(f"unexpected summary: {payload['summary']}", file=sys.stderr)
            return 1
        if "engine_reference_scores" not in payload["summary"]:
            print(f"missing candidate-score summary: {payload['summary']}", file=sys.stderr)
            return 1
        if "engine_played_negative_see" not in payload["summary"]:
            print(f"missing SEE summary: {payload['summary']}", file=sys.stderr)
            return 1
        if "engine_prefers_negative_see" not in payload["summary"]:
            print(f"missing SEE preference summary: {payload['summary']}", file=sys.stderr)
            return 1
        if not rows:
            print("expected at least one CSV event", file=sys.stderr)
            return 1
        required_csv_fields = {
            "engine_played_score_cp",
            "engine_played_see_cp",
            "engine_played_pv",
            "engine_reference_score_cp",
            "engine_reference_see_cp",
            "engine_reference_delta_cp",
            "engine_reference_pv",
        }
        if not required_csv_fields.issubset(rows[0].keys()):
            print(f"missing CSV fields: {required_csv_fields - set(rows[0].keys())}", file=sys.stderr)
            return 1
        if "bad-trade-sequence" not in report or "Sequence after reply" not in report:
            print(report, file=sys.stderr)
            return 1
        if "Engine played-move constrained score" not in report:
            print(report, file=sys.stderr)
            return 1
        if "Engine played-move SEE" not in report:
            print(report, file=sys.stderr)
            return 1
        if "Engine played-move PV" not in report:
            print(report, file=sys.stderr)
            return 1
        if "Negative SEE Capture Audit" not in report:
            print(report, file=sys.stderr)
            return 1
        if "postmortem-0001" not in positions or " bm " not in positions:
            print(positions, file=sys.stderr)
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
