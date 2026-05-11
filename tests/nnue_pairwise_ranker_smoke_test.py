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
]


def write_dataset(path: Path) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=["fen", "score_cp", "source"])
        writer.writeheader()
        for index, (fen, score) in enumerate(POSITIONS, start=1):
            writer.writerow({"fen": fen, "score_cp": score, "source": f"smoke:{index}"})


def write_epd(path: Path) -> None:
    rows = [
        '4k3/8/8/8/8/8/4Q3/4K3 w - - bm e2e8; am e2e7; id "white-rank"; theme "rank-smoke"; c0 "severity 400; ref_delta 300";',
        '4k3/4q3/8/8/8/8/8/4K3 b - - bm e7e1; am e7e2; id "black-rank"; theme "rank-smoke"; c0 "severity 400; ref_delta 300";',
    ]
    path.write_text("\n".join(rows) + "\n", encoding="utf-8")


def run(command: list[str]) -> int:
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        print(completed.stdout)
        print(completed.stderr, file=sys.stderr)
    return completed.returncode


def count_rows(path: Path) -> int:
    with path.open(encoding="utf-8", newline="") as handle:
        return max(0, sum(1 for _ in handle) - 1)


def main() -> int:
    if len(sys.argv) != 5:
        print(
            "usage: nnue_pairwise_ranker_smoke_test.py <cache_script> <train_script> <pair_script> <ranker_script>",
            file=sys.stderr,
        )
        return 2
    cache_script = Path(sys.argv[1])
    train_script = Path(sys.argv[2])
    pair_script = Path(sys.argv[3])
    ranker_script = Path(sys.argv[4])

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        dataset = root / "positions.csv"
        cache = root / "cache.pt"
        checkpoint = root / "base.pt"
        epd = root / "pairs.epd"
        pairs = root / "pairs.csv"
        validation_pairs = root / "validation_pairs.csv"
        tuned = root / "tuned.pt"
        tuned_nnue = root / "tuned.nnue"
        write_dataset(dataset)
        write_epd(epd)

        commands = [
            [
                sys.executable,
                str(cache_script),
                "--dataset",
                str(dataset),
                "--output",
                str(cache),
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
                str(cache),
                "--validation-cache",
                str(cache),
                "--output",
                str(checkpoint),
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
            ],
            [
                sys.executable,
                str(pair_script),
                "--epd",
                str(epd),
                "--output",
                str(pairs),
                "--validation-output",
                str(validation_pairs),
                "--validation-fraction",
                "0.5",
                "--extra-legal-negatives",
                "1",
            ],
            [
                sys.executable,
                str(ranker_script),
                "--checkpoint",
                str(checkpoint),
                "--pairs",
                str(pairs),
                "--validation-pairs",
                str(validation_pairs),
                "--output",
                str(tuned),
                "--export",
                str(tuned_nnue),
                "--epochs",
                "1",
                "--batch-size",
                "2",
                "--learning-rate",
                "0.0001",
                "--progress-every",
                "1",
            ],
        ]
        for command in commands:
            code = run(command)
            if code != 0:
                return code
        if count_rows(pairs) == 0 or count_rows(validation_pairs) == 0:
            print("pair builder did not write expected train/validation rows", file=sys.stderr)
            return 1
        for path in (checkpoint, tuned, tuned_nnue):
            if not path.exists():
                print(f"missing expected ranker artifact: {path}", file=sys.stderr)
                return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
