#!/usr/bin/env python3
from __future__ import annotations

import csv
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    import chess  # noqa: F401
    import torch  # noqa: F401
except ImportError:
    print("python-chess or torch not installed; skipping")
    raise SystemExit(0)


POSITIONS = [
    ("8/8/8/8/8/8/4K3/4k3 w - - 0 1", 0),
    ("8/8/8/8/8/8/4K3/4k3 b - - 0 1", 0),
    ("8/8/8/8/8/8/3QK3/4k3 w - - 0 1", 900),
    ("8/8/8/8/8/8/4K3/3qk3 b - - 0 1", -900),
    ("8/8/8/8/8/8/3RK3/4k3 w - - 0 1", 500),
    ("8/8/8/8/8/8/4K3/3rk3 b - - 0 1", -500),
]


def write_dataset(path: Path, rows: list[tuple[str, int]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=["fen", "score_cp", "source"])
        writer.writeheader()
        for index, (fen, score) in enumerate(rows, start=1):
            writer.writerow({"fen": fen, "score_cp": score, "source": f"smoke:{index}"})


def run(command: list[str]) -> int:
    result = subprocess.run(command, text=True, capture_output=True, check=False)
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
    return result.returncode


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: nnue_feature_cache_smoke_test.py <cache_script> <train_script>", file=sys.stderr)
        return 2
    cache_script = Path(sys.argv[1])
    train_script = Path(sys.argv[2])
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        train_csv = root / "train.csv"
        validation_csv = root / "validation.csv"
        write_dataset(train_csv, POSITIONS[:4])
        write_dataset(validation_csv, POSITIONS[4:])
        train_cache = root / "train_cache.pt"
        validation_cache = root / "validation_cache.pt"
        checkpoint = root / "model.pt"
        commands = [
            [
                sys.executable,
                str(cache_script),
                "--dataset",
                str(train_csv),
                "--output",
                str(train_cache),
                "--progress-every",
                "1",
            ],
            [
                sys.executable,
                str(cache_script),
                "--dataset",
                str(validation_csv),
                "--output",
                str(validation_cache),
                "--progress-every",
                "1",
            ],
            [
                sys.executable,
                str(train_script),
                "--train-cache",
                str(train_cache),
                "--validation-cache",
                str(validation_cache),
                "--output",
                str(checkpoint),
                "--epochs",
                "1",
                "--batch-size",
                "2",
                "--hidden-size",
                "8",
                "--batch-progress-every",
                "1",
                "--max-train-rows",
                "3",
                "--max-validation-rows",
                "2",
            ],
        ]
        for command in commands:
            code = run(command)
            if code != 0:
                return code
        if not checkpoint.exists():
            print("checkpoint was not written", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
