#!/usr/bin/env python3
from __future__ import annotations

import csv
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    import chess  # noqa: F401
except ImportError:
    print("python-chess not installed; skipping")
    raise SystemExit(0)


def write_inputs(directory: Path) -> tuple[Path, Path]:
    train = directory / "train.csv"
    test = directory / "test.csv"
    with train.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=["FEN", "Evaluation"])
        writer.writeheader()
        writer.writerow({"FEN": "8/8/8/8/8/8/4K3/4k3 w - - 0 1", "Evaluation": "37"})
        writer.writerow({"FEN": "8/8/8/8/8/8/4K3/4k3 b - - 0 1", "Evaluation": "-42"})
        writer.writerow({"FEN": "8/8/8/8/8/8/4K3/4k3 w - - 0 1", "Evaluation": "37"})
        writer.writerow({"FEN": "invalid", "Evaluation": "12"})
        writer.writerow({"FEN": "8/8/8/8/8/8/3QK3/4k3 w - - 0 1", "Evaluation": "2500"})
    with test.open("w", encoding="utf-8", newline="") as handle:
        handle.write("FEN\tEvaluation\n")
        handle.write("8/8/8/8/8/8/4K3/4k3 w - - 0 1\t\n")
        handle.write("8/8/8/8/8/8/4K3/4k3 b - - 0 1\t\n")
    return train, test


def row_count(path: Path) -> int:
    with path.open(encoding="utf-8", newline="") as handle:
        return sum(1 for _ in csv.DictReader(handle))


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: import_kaggle_stockfish_nnue_smoke_test.py <import_script>", file=sys.stderr)
        return 2
    script = Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        train, test = write_inputs(root)
        output = root / "out"
        result = subprocess.run(
            [
                sys.executable,
                str(script),
                "--train-input",
                str(train),
                "--test-input",
                str(test),
                "--output-dir",
                str(output),
                "--clip-cp",
                "1500",
                "--validation-fraction",
                "0.20",
                "--holdout-fraction",
                "0.20",
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
        labeled_rows = row_count(output / "train.csv") + row_count(output / "validation.csv") + row_count(output / "holdout.csv")
        if labeled_rows != 3:
            print(f"expected 3 unique labeled rows, got {labeled_rows}", file=sys.stderr)
            return 1
        if row_count(output / "unlabeled_test.csv") != 2:
            print("expected 2 unlabeled rows", file=sys.stderr)
            return 1
        report = (output / "import_report.txt").read_text(encoding="utf-8")
        for expected in ("duplicate_labeled 1", "invalid_labeled 1", "clipped_labeled 1"):
            if expected not in report:
                print(f"missing report line: {expected}", file=sys.stderr)
                return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
