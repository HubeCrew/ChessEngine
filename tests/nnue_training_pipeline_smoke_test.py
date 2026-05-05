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


def require_exists(paths: list[Path]) -> int:
    for path in paths:
        if not path.exists():
            print(f"missing expected pipeline artifact: {path}", file=sys.stderr)
            return 1
    return 0


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: nnue_training_pipeline_smoke_test.py <cache_script> <train_script> <engine>", file=sys.stderr)
        return 2
    cache_script = Path(sys.argv[1])
    train_script = Path(sys.argv[2])
    engine = Path(sys.argv[3])
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        train_csv = root / "train.csv"
        validation_csv = root / "validation.csv"
        write_dataset(train_csv, POSITIONS[:4])
        write_dataset(validation_csv, POSITIONS[4:])
        train_cache = root / "train_cache.pt"
        validation_cache = root / "validation_cache.pt"
        run_dir = root / "run"

        commands = [
            [
                sys.executable,
                str(cache_script),
                "--dataset",
                str(train_csv),
                "--output",
                str(train_cache),
                "--feature-set",
                "halfka-v2-hm-full-threats",
                "--max-features",
                "0",
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
                "--feature-set",
                "halfka-v2-hm-full-threats",
                "--max-features",
                "0",
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
                "--run-dir",
                str(run_dir),
                "--epochs",
                "1",
                "--batch-size",
                "2",
                "--hidden-size",
                "8",
                "--architecture",
                "sf-lite",
                "--l2-size",
                "4",
                "--l3-size",
                "4",
                "--batch-progress-every",
                "1",
                "--holdout-dataset",
                str(validation_csv),
                "--export-best",
                "--parity-engine",
                str(engine),
                "--parity-limit",
                "2",
            ],
            [
                sys.executable,
                str(train_script),
                "--resume",
                str(run_dir / "last.pt"),
                "--epochs",
                "2",
                "--holdout-dataset",
                str(validation_csv),
                "--export-best",
                "--parity-engine",
                str(engine),
                "--parity-limit",
                "2",
            ],
        ]
        for command in commands:
            code = run(command)
            if code != 0:
                return code
        return require_exists(
            [
                run_dir / "config.json",
                run_dir / "metrics.csv",
                run_dir / "metrics.jsonl",
                run_dir / "last.pt",
                run_dir / "best.pt",
                run_dir / "best.nnue",
                run_dir / "holdout_report.txt",
                run_dir / "holdout_metrics.json",
                run_dir / "parity_report.txt",
            ]
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
