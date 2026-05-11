#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def write_seed(path: Path) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=["fen", "score_cp", "source"])
        writer.writeheader()
        writer.writerow(
            {
                "fen": "4k3/8/8/8/8/8/4K3/8 w - - 0 1",
                "score_cp": "12",
                "source": "seed:one",
            }
        )


def write_lichess_jsonl(path: Path) -> None:
    rows = [
        {
            "fen": "4k3/8/8/8/8/8/4K3/8 w - -",
            "evals": [{"depth": 16, "knodes": 100, "pvs": [{"cp": 20}]}],
        },
        {
            "fen": "4k3/8/8/8/8/8/4K3/8 b - -",
            "evals": [{"depth": 10, "knodes": 100, "pvs": [{"cp": -21}]}],
        },
        {
            "fen": "4k3/8/8/8/8/8/3QK3/8 w - -",
            "evals": [
                {"depth": 12, "knodes": 50, "pvs": [{"cp": 800}]},
                {"depth": 18, "knodes": 50, "pvs": [{"cp": 2500}]},
            ],
        },
        {
            "fen": "rnbqkbnr/pppppppp/rrrrrrrr/8/8/RRRRRRRR/PPPPPPPP/RNBQKBNR w KQkq -",
            "evals": [{"depth": 18, "knodes": 100, "pvs": [{"cp": 15}]}],
        },
        {
            "fen": "4k3/8/8/8/8/8/4K3/3q4 b - -",
            "evals": [{"depth": 18, "knodes": 100, "pvs": [{"mate": -3}]}],
        },
        {
            "fen": "4k3/8/8/8/8/8/4K3/8 b - -",
            "evals": [{"depth": 18, "knodes": 100, "pvs": [{"cp": -30}]}],
        },
    ]
    with path.open("w", encoding="utf-8") as handle:
        for row in rows:
            handle.write(json.dumps(row) + "\n")
        handle.write("{bad json}\n")


def count_rows(path: Path) -> int:
    with path.open(encoding="utf-8", newline="") as handle:
        return sum(1 for _ in csv.DictReader(handle))


def read_scores(output: Path) -> list[int]:
    scores: list[int] = []
    for name in ("train.csv", "validation.csv", "holdout.csv"):
        with (output / name).open(encoding="utf-8", newline="") as handle:
            scores.extend(int(row["score_cp"]) for row in csv.DictReader(handle))
    return scores


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: import_lichess_evaluations_smoke_test.py <import_script>", file=sys.stderr)
        return 2
    script = Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        seed = root / "seed.csv"
        lichess = root / "lichess.jsonl"
        output = root / "out"
        write_seed(seed)
        write_lichess_jsonl(lichess)
        result = subprocess.run(
            [
                sys.executable,
                str(script),
                "--input",
                str(lichess),
                "--seed-dataset",
                str(seed),
                "--output-dir",
                str(output),
                "--target-total-rows",
                "4",
                "--min-depth",
                "12",
                "--clip-cp",
                "1500",
                "--stratify",
                "none",
                "--progress-every",
                "1",
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode != 0:
            print(result.stdout)
            print(result.stderr, file=sys.stderr)
            return result.returncode
        total_rows = sum(count_rows(output / name) for name in ("train.csv", "validation.csv", "holdout.csv"))
        if total_rows != 4:
            print(f"expected 4 total rows, got {total_rows}", file=sys.stderr)
            return 1
        scores = sorted(read_scores(output))
        if scores != [-1500, -30, 12, 1500]:
            print(f"unexpected scores: {scores}", file=sys.stderr)
            return 1
        report = (output / "import_report.txt").read_text(encoding="utf-8")
        for expected in (
            "seed_written 1",
            "lichess_written 3",
            "lichess_duplicate 1",
            "lichess_no_usable_eval 1",
            "lichess_nonstandard_material 1",
        ):
            if expected not in report:
                print(f"missing report line: {expected}", file=sys.stderr)
                return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
