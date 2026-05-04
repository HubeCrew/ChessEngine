#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

import torch

from nnue_model import DEFAULT_ACCUMULATOR_SCALE, DEFAULT_OUTPUT_SCALE, export_binary, load_checkpoint


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export a trained PyTorch NNUE checkpoint to the engine binary format.")
    parser.add_argument("--checkpoint", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--accumulator-scale", type=int, default=DEFAULT_ACCUMULATOR_SCALE)
    parser.add_argument("--output-scale", type=int, default=DEFAULT_OUTPUT_SCALE)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    device = torch.device("cpu")
    print(f"[export] loading checkpoint={args.checkpoint}")
    model, metadata = load_checkpoint(args.checkpoint, device)
    print(
        f"[export] exporting output={args.output} hidden={model.hidden_size} "
        f"feature_set={model.feature_set} feature_count={model.feature_count} "
        f"accumulator_scale={args.accumulator_scale} output_scale={args.output_scale}"
    )
    export_binary(args.output, model, args.accumulator_scale, args.output_scale)
    print(f"wrote {args.output} hidden={model.hidden_size} metadata={metadata}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
