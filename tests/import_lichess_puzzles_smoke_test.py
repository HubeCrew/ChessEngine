import csv
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: import_lichess_puzzles_smoke_test.py <import_script>", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory() as directory:
        tempdir = Path(directory)
        csv_path = tempdir / "puzzles.csv"
        epd_path = tempdir / "imported.epd"

        with csv_path.open("w", newline="", encoding="utf-8") as output:
            writer = csv.DictWriter(
                output,
                fieldnames=[
                    "PuzzleId",
                    "FEN",
                    "Moves",
                    "Rating",
                    "RatingDeviation",
                    "Popularity",
                    "NbPlays",
                    "Themes",
                    "GameUrl",
                    "OpeningTags",
                ],
            )
            writer.writeheader()
            writer.writerow(
                {
                    "PuzzleId": "abc123",
                    "FEN": "4k3/7q/8/8/8/8/8/4K2R b - - 0 1",
                    "Moves": "h7h2 h1h2 e8f7",
                    "Rating": "1200",
                    "RatingDeviation": "70",
                    "Popularity": "90",
                    "NbPlays": "1000",
                    "Themes": "hangingPiece middlegame short",
                    "GameUrl": "https://lichess.org/test#1",
                    "OpeningTags": "",
                }
            )

        completed = subprocess.run(
            [
                sys.argv[1],
                "--input",
                str(csv_path),
                "--output",
                str(epd_path),
                "--limit",
                "1",
                "--min-solution-plies",
                "2",
                "--referee",
                str(Path(__file__).resolve().parents[1] / "build" / "chess_referee"),
            ],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=10,
        )
        if completed.returncode != 0:
            print(completed.stdout, file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return completed.returncode

        epd = epd_path.read_text(encoding="utf-8")
        required = [
            "4k3/8/8/8/8/8/7q/4K2R w - - bm h1h2;",
            'id "lichess-abc123";',
            'theme "hangingPiece";',
            'pv "h1h2 e8f7";',
            'rating 1200;',
        ]
        for text in required:
            if text not in epd:
                print(f"missing expected text {text!r} in:\n{epd}", file=sys.stderr)
                return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
