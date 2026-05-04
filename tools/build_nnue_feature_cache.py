#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

import chess
import torch

from nnue_model import FEATURE_SET_HALFKA_V2_HM_THREAT_LITE, SUPPORTED_FEATURE_SETS, active_features, feature_count_for


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build a compact tensor feature cache for large NNUE CSV datasets.")
    parser.add_argument("--dataset", required=True, type=Path, help="Canonical CSV with fen and score_cp columns.")
    parser.add_argument("--output", required=True, type=Path, help="Output .pt feature cache.")
    parser.add_argument("--clip-cp", type=int, default=1500)
    parser.add_argument("--max-features", type=int, default=30, help="Maximum active features per perspective; 0 selects a feature-set default.")
    parser.add_argument(
        "--feature-set",
        choices=SUPPORTED_FEATURE_SETS,
        default="halfkp-v1",
        help="NNUE feature mapping to encode.",
    )
    parser.add_argument("--progress-every", type=int, default=100000)
    return parser.parse_args()


def count_rows(path: Path) -> int:
    with path.open(encoding="utf-8", newline="") as handle:
        return max(0, sum(1 for _ in handle) - 1)


def main() -> int:
    args = parse_args()
    if args.clip_cp <= 0:
        print("--clip-cp must be positive", file=sys.stderr)
        return 2
    if args.max_features < 0:
        print("--max-features must be non-negative", file=sys.stderr)
        return 2
    if args.max_features == 0:
        args.max_features = 160 if args.feature_set == FEATURE_SET_HALFKA_V2_HM_THREAT_LITE else 30
    if args.progress_every <= 0:
        print("--progress-every must be positive", file=sys.stderr)
        return 2

    total_input = count_rows(args.dataset)
    print(
        f"[cache] allocating rows={total_input} max_features={args.max_features} "
        f"feature_set={args.feature_set} feature_count={feature_count_for(args.feature_set)} dataset={args.dataset}",
        file=sys.stderr,
        flush=True,
    )
    white_indices = torch.zeros((total_input, args.max_features), dtype=torch.int32)
    black_indices = torch.zeros((total_input, args.max_features), dtype=torch.int32)
    white_lengths = torch.zeros(total_input, dtype=torch.int16)
    black_lengths = torch.zeros(total_input, dtype=torch.int16)
    side_to_move = torch.empty(total_input, dtype=torch.int8)
    target = torch.empty(total_input, dtype=torch.float32)

    written = 0
    invalid = 0
    too_wide = 0
    with args.dataset.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for input_index, row in enumerate(reader, start=1):
            try:
                board = chess.Board(row["fen"])
                score = max(-args.clip_cp, min(args.clip_cp, float(row["score_cp"])))
                white = active_features(board, chess.WHITE, args.feature_set)
                black = active_features(board, chess.BLACK, args.feature_set)
            except (KeyError, ValueError):
                invalid += 1
                continue
            if len(white) > args.max_features or len(black) > args.max_features:
                too_wide += 1
                continue
            if white:
                white_indices[written, : len(white)] = torch.tensor(white, dtype=torch.int32)
            if black:
                black_indices[written, : len(black)] = torch.tensor(black, dtype=torch.int32)
            white_lengths[written] = len(white)
            black_lengths[written] = len(black)
            side_to_move[written] = 1 if board.turn == chess.WHITE else -1
            target[written] = score
            written += 1
            if input_index % args.progress_every == 0 or input_index == total_input:
                print(
                    f"[cache] processed={input_index}/{total_input} cached={written} "
                    f"invalid={invalid} too_wide={too_wide}",
                    file=sys.stderr,
                    flush=True,
                )

    if written == 0:
        print("no usable positions cached", file=sys.stderr)
        return 1

    cache = {
        "format_version": 3,
        "feature_set": args.feature_set,
        "feature_count": feature_count_for(args.feature_set),
        "dataset": str(args.dataset),
        "rows": written,
        "max_features": args.max_features,
        "clip_cp": args.clip_cp,
        "white_indices": white_indices[:written].contiguous(),
        "white_lengths": white_lengths[:written].contiguous(),
        "black_indices": black_indices[:written].contiguous(),
        "black_lengths": black_lengths[:written].contiguous(),
        "side_to_move": side_to_move[:written].contiguous(),
        "target": target[:written].contiguous(),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    print(f"[cache] saving output={args.output} rows={written}", file=sys.stderr, flush=True)
    torch.save(cache, args.output)
    print(f"wrote feature cache {args.output} rows={written} invalid={invalid} too_wide={too_wide}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
