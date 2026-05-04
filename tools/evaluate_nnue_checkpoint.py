#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import math
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

import chess
import torch

from nnue_model import NnueModel, active_features, load_checkpoint, make_batch


BUCKETS = ("equal", "small", "clear", "large", "decisive")


@dataclass
class Stats:
    count: int = 0
    abs_error: float = 0.0
    squared_error: float = 0.0
    signed_error: float = 0.0
    prediction_sum: float = 0.0
    target_sum: float = 0.0
    prediction_squared_sum: float = 0.0
    target_squared_sum: float = 0.0
    product_sum: float = 0.0

    def add(self, prediction: float, target: float) -> None:
        error = prediction - target
        self.count += 1
        self.abs_error += abs(error)
        self.squared_error += error * error
        self.signed_error += error
        self.prediction_sum += prediction
        self.target_sum += target
        self.prediction_squared_sum += prediction * prediction
        self.target_squared_sum += target * target
        self.product_sum += prediction * target

    def mae(self) -> float:
        return self.abs_error / max(1, self.count)

    def rmse(self) -> float:
        return math.sqrt(self.squared_error / max(1, self.count))

    def bias(self) -> float:
        return self.signed_error / max(1, self.count)

    def correlation(self) -> float:
        total = max(1, self.count)
        covariance = self.product_sum - (self.prediction_sum * self.target_sum / total)
        prediction_variance = self.prediction_squared_sum - (self.prediction_sum * self.prediction_sum / total)
        target_variance = self.target_squared_sum - (self.target_sum * self.target_sum / total)
        if prediction_variance <= 0.0 or target_variance <= 0.0:
            return 0.0
        return covariance / math.sqrt(prediction_variance * target_variance)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Evaluate an NNUE checkpoint against a labeled CSV with bucketed metrics.")
    parser.add_argument("--checkpoint", required=True, type=Path)
    parser.add_argument("--dataset", required=True, type=Path, help="CSV with fen and score_cp columns.")
    parser.add_argument("--batch-size", type=int, default=4096)
    parser.add_argument("--max-rows", type=int, default=0)
    parser.add_argument("--progress-every", type=int, default=25000)
    return parser.parse_args()


def score_bucket(score: float) -> str:
    value = abs(score)
    if value < 50:
        return "equal"
    if value < 150:
        return "small"
    if value < 350:
        return "clear"
    if value < 800:
        return "large"
    return "decisive"


def phase_bucket(board: chess.Board) -> str:
    non_pawn = sum(
        len(board.pieces(piece_type, color))
        for piece_type in (chess.KNIGHT, chess.BISHOP, chess.ROOK, chess.QUEEN)
        for color in (chess.WHITE, chess.BLACK)
    )
    if non_pawn >= 12:
        return "opening/middlegame"
    if non_pawn >= 6:
        return "middlegame/endgame"
    return "endgame"


def evaluate_batch(
    model: NnueModel,
    device: torch.device,
    items: list[tuple[list[int], list[int], int, float]],
) -> list[float]:
    batch = make_batch(items, device)
    with torch.no_grad():
        prediction = model(batch.white, batch.white_mask, batch.black, batch.black_mask, batch.side_to_move)
    return [float(value) for value in prediction.detach().cpu()]


def add_group(groups: dict[str, Stats], name: str, prediction: float, target: float) -> None:
    groups[name].add(prediction, target)


def print_group(name: str, stats: Stats) -> None:
    print(
        f"{name:32s} count={stats.count:7d} mae={stats.mae():7.2f} rmse={stats.rmse():7.2f} "
        f"bias={stats.bias():8.2f} corr={stats.correlation():6.3f}"
    )


def main() -> int:
    args = parse_args()
    if args.batch_size <= 0:
        print("--batch-size must be positive", file=sys.stderr)
        return 2
    if args.max_rows < 0:
        print("--max-rows must not be negative", file=sys.stderr)
        return 2
    if args.progress_every <= 0:
        print("--progress-every must be positive", file=sys.stderr)
        return 2

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"[eval] loading checkpoint={args.checkpoint}", file=sys.stderr, flush=True)
    model, metadata = load_checkpoint(args.checkpoint, device)
    print(f"[eval] hidden={model.hidden_size} device={device} metadata={metadata}", file=sys.stderr, flush=True)

    groups: dict[str, Stats] = defaultdict(Stats)
    batch_items: list[tuple[list[int], list[int], int, float]] = []
    batch_groups: list[list[str]] = []
    processed = 0
    invalid = 0

    def flush() -> None:
        nonlocal batch_items, batch_groups
        if not batch_items:
            return
        predictions = evaluate_batch(model, device, batch_items)
        for prediction, item_groups, item in zip(predictions, batch_groups, batch_items):
            target = item[3]
            for group in item_groups:
                add_group(groups, group, prediction, target)
        batch_items = []
        batch_groups = []

    with args.dataset.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            if args.max_rows > 0 and processed >= args.max_rows:
                break
            try:
                board = chess.Board(row["fen"])
                target = float(row["score_cp"])
            except (KeyError, ValueError):
                invalid += 1
                continue

            side = 1 if board.turn == chess.WHITE else -1
            batch_items.append((active_features(board, chess.WHITE), active_features(board, chess.BLACK), side, target))
            batch_groups.append(
                [
                    "all",
                    f"score:{score_bucket(target)}",
                    "side:white" if board.turn == chess.WHITE else "side:black",
                    f"phase:{phase_bucket(board)}",
                ]
            )
            processed += 1
            if len(batch_items) >= args.batch_size:
                flush()
            if processed % args.progress_every == 0:
                print(f"[eval] processed={processed} invalid={invalid}", file=sys.stderr, flush=True)
    flush()

    if groups["all"].count == 0:
        print("no usable rows", file=sys.stderr)
        return 1

    print(f"checkpoint {args.checkpoint}")
    print(f"dataset {args.dataset}")
    print(f"rows {processed} invalid {invalid}")
    print_group("all", groups["all"])
    for bucket in BUCKETS:
        print_group(f"score:{bucket}", groups[f"score:{bucket}"])
    for side in ("white", "black"):
        print_group(f"side:{side}", groups[f"side:{side}"])
    for phase in ("opening/middlegame", "middlegame/endgame", "endgame"):
        print_group(f"phase:{phase}", groups[f"phase:{phase}"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
