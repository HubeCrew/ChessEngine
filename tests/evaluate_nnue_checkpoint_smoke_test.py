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
    ("8/8/8/8/8/8/3QK3/4k3 w - - 0 1", 900),
    ("8/8/8/8/8/8/4K3/3qk3 b - - 0 1", -900),
]


def write_dataset(path: Path) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=["fen", "score_cp", "source"])
        writer.writeheader()
        for index, (fen, score) in enumerate(POSITIONS, start=1):
            writer.writerow({"fen": fen, "score_cp": score, "source": f"smoke:{index}"})


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: evaluate_nnue_checkpoint_smoke_test.py <evaluate_script> <tools_dir>", file=sys.stderr)
        return 2
    evaluate_script = Path(sys.argv[1])
    tools_dir = Path(sys.argv[2])
    sys.path.insert(0, str(tools_dir))
    from nnue_model import NnueModel, save_checkpoint  # noqa: PLC0415

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        dataset = root / "dataset.csv"
        checkpoint = root / "model.pt"
        write_dataset(dataset)
        save_checkpoint(checkpoint, NnueModel(hidden_size=8), {"test": "smoke"})
        result = subprocess.run(
            [
                sys.executable,
                str(evaluate_script),
                "--checkpoint",
                str(checkpoint),
                "--dataset",
                str(dataset),
                "--batch-size",
                "2",
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
        for expected in ("all", "score:equal", "score:decisive", "side:white", "side:black"):
            if expected not in result.stdout:
                print(f"missing output group {expected}", file=sys.stderr)
                print(result.stdout)
                return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
