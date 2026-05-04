#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import random
import sys
import time
from pathlib import Path
from typing import Any

import chess
import torch
from torch import nn
from torch.utils.data import DataLoader, Dataset

from nnue_model import Batch, NnueModel, active_features, make_batch, save_checkpoint


class NnueDataset(Dataset):
    def __init__(self, rows: list[tuple[str, float]], progress_every: int) -> None:
        self.items: list[tuple[list[int], list[int], int, float]] = []
        for index, (fen, score) in enumerate(rows, start=1):
            board = chess.Board(fen)
            side_to_move = 1 if board.turn == chess.WHITE else -1
            self.items.append((active_features(board, chess.WHITE), active_features(board, chess.BLACK), side_to_move, score))
            if index % progress_every == 0 or index == len(rows):
                print(f"[train] encoded={index}/{len(rows)}", file=sys.stderr, flush=True)

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, index: int) -> tuple[list[int], list[int], int, float]:
        return self.items[index]


class CachedNnueDataset(Dataset):
    def __init__(self, path: Path, row_limit: int = 0) -> None:
        print(f"[train] loading feature_cache={path}", file=sys.stderr, flush=True)
        self.path = path
        self.cache = torch.load(path, map_location="cpu", weights_only=True)
        if int(self.cache.get("format_version", 0)) != 2:
            raise ValueError(f"unsupported feature cache format in {path}")
        required = ("white_indices", "white_lengths", "black_indices", "black_lengths", "side_to_move", "target")
        for key in required:
            if key not in self.cache:
                raise ValueError(f"feature cache {path} is missing {key}")
        rows = int(self.cache["target"].numel())
        if rows == 0:
            raise ValueError(f"feature cache {path} is empty")
        if row_limit > 0:
            rows = min(rows, row_limit)
            for key in ("white_indices", "white_lengths", "black_indices", "black_lengths", "side_to_move", "target"):
                self.cache[key] = self.cache[key][:rows].contiguous()
        print(f"[train] loaded feature_cache={path} rows={rows}", file=sys.stderr, flush=True)

    def __len__(self) -> int:
        return int(self.cache["target"].numel())

    def __getitem__(self, index: int) -> tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]:
        return (
            self.cache["white_indices"][index],
            self.cache["white_lengths"][index],
            self.cache["black_indices"][index],
            self.cache["black_lengths"][index],
            self.cache["side_to_move"][index],
            self.cache["target"][index],
        )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train a small HalfKP-style NNUE model from labeled CSV positions.")
    parser.add_argument("--dataset", type=Path, help="CSV with fen and score_cp columns. Uses --validation-split.")
    parser.add_argument("--train-cache", type=Path, help="Feature cache built by build_nnue_feature_cache.py.")
    parser.add_argument("--validation-cache", type=Path, help="Validation feature cache built by build_nnue_feature_cache.py.")
    parser.add_argument("--output", required=True, type=Path, help="Output PyTorch checkpoint.")
    parser.add_argument("--hidden-size", type=int, default=256)
    parser.add_argument("--epochs", type=int, default=5)
    parser.add_argument("--batch-size", type=int, default=512)
    parser.add_argument("--learning-rate", type=float, default=0.001)
    parser.add_argument(
        "--loss-mode",
        choices=("raw", "shaped", "combined"),
        default="raw",
        help="Training objective. raw uses centipawn SmoothL1. shaped uses tanh-compressed centipawns. combined adds both.",
    )
    parser.add_argument("--target-scale", type=float, default=600.0, help="Centipawn scale for tanh target shaping.")
    parser.add_argument("--shaped-weight", type=float, default=1.0, help="Multiplier for shaped loss in combined mode.")
    parser.add_argument("--validation-split", type=float, default=0.05)
    parser.add_argument("--clip-cp", type=int, default=1500)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--max-train-rows", type=int, default=0, help="Optional smoke-test cap. 0 means no cap.")
    parser.add_argument("--max-validation-rows", type=int, default=0, help="Optional smoke-test cap. 0 means no cap.")
    parser.add_argument("--progress-every", type=int, default=5000, help="Feature-encoding progress interval.")
    parser.add_argument("--batch-progress-every", type=int, default=25, help="Training-batch progress interval.")
    return parser.parse_args()


def shaped_cp(values: torch.Tensor, target_scale: float) -> torch.Tensor:
    return torch.tanh(values / target_scale) * target_scale


def objective_loss(
    prediction: torch.Tensor,
    target: torch.Tensor,
    raw_loss_fn: nn.SmoothL1Loss,
    loss_mode: str,
    target_scale: float,
    shaped_weight: float,
) -> torch.Tensor:
    raw_loss = raw_loss_fn(prediction, target)
    if loss_mode == "raw":
        return raw_loss
    shaped_loss = raw_loss_fn(shaped_cp(prediction, target_scale), shaped_cp(target, target_scale))
    if loss_mode == "shaped":
        return shaped_loss
    return raw_loss + shaped_weight * shaped_loss


def load_rows(path: Path, clip_cp: int) -> list[tuple[str, float]]:
    print(f"[train] loading dataset={path}", file=sys.stderr, flush=True)
    rows: list[tuple[str, float]] = []
    with path.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            fen = row["fen"]
            score = max(-clip_cp, min(clip_cp, float(row["score_cp"])))
            rows.append((fen, score))
    print(f"[train] loaded rows={len(rows)}", file=sys.stderr, flush=True)
    return rows


def collate_csv(items: list[tuple[list[int], list[int], int, float]], device: torch.device) -> Batch:
    return make_batch(items, device)


def collate_cache(items: list[tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]], device: torch.device) -> Batch:
    white_indices = torch.stack([item[0] for item in items]).to(device=device, dtype=torch.long)
    white_lengths = torch.stack([item[1] for item in items]).to(device=device, dtype=torch.long)
    black_indices = torch.stack([item[2] for item in items]).to(device=device, dtype=torch.long)
    black_lengths = torch.stack([item[3] for item in items]).to(device=device, dtype=torch.long)
    side_to_move = torch.stack([item[4] for item in items]).to(device=device, dtype=torch.float32)
    target = torch.stack([item[5] for item in items]).to(device=device, dtype=torch.float32)

    white_range = torch.arange(white_indices.shape[1], device=device).unsqueeze(0)
    black_range = torch.arange(black_indices.shape[1], device=device).unsqueeze(0)
    white_mask = (white_range < white_lengths.unsqueeze(1)).to(torch.float32)
    black_mask = (black_range < black_lengths.unsqueeze(1)).to(torch.float32)
    return Batch(
        white=white_indices,
        white_mask=white_mask,
        black=black_indices,
        black_mask=black_mask,
        side_to_move=side_to_move,
        target=target,
    )


def evaluate(
    model: NnueModel,
    loader: DataLoader,
    device: torch.device,
    loss_mode: str,
    target_scale: float,
    shaped_weight: float,
) -> dict[str, float]:
    loss_fn = nn.SmoothL1Loss()
    total_objective = 0.0
    total_raw_loss = 0.0
    total_abs = 0.0
    total_squared = 0.0
    sum_prediction = 0.0
    sum_target = 0.0
    sum_prediction_squared = 0.0
    sum_target_squared = 0.0
    sum_product = 0.0
    total = 0
    model.eval()
    with torch.no_grad():
        for batch in loader:
            prediction = model(batch.white, batch.white_mask, batch.black, batch.black_mask, batch.side_to_move)
            objective = objective_loss(prediction, batch.target, loss_fn, loss_mode, target_scale, shaped_weight)
            raw_loss = loss_fn(prediction, batch.target)
            batch_size = int(batch.target.numel())
            total_objective += float(objective) * batch_size
            total_raw_loss += float(raw_loss) * batch_size
            diff = prediction - batch.target
            total_abs += float(diff.abs().sum())
            total_squared += float((diff * diff).sum())
            sum_prediction += float(prediction.sum())
            sum_target += float(batch.target.sum())
            sum_prediction_squared += float((prediction * prediction).sum())
            sum_target_squared += float((batch.target * batch.target).sum())
            sum_product += float((prediction * batch.target).sum())
            total += batch_size

    total = max(1, total)
    covariance = sum_product - (sum_prediction * sum_target / total)
    prediction_variance = sum_prediction_squared - (sum_prediction * sum_prediction / total)
    target_variance = sum_target_squared - (sum_target * sum_target / total)
    if prediction_variance <= 0.0 or target_variance <= 0.0:
        correlation = 0.0
    else:
        correlation = covariance / ((prediction_variance * target_variance) ** 0.5)
    return {
        "objective": total_objective / total,
        "raw_loss": total_raw_loss / total,
        "mae": total_abs / total,
        "rmse": (total_squared / total) ** 0.5,
        "correlation": correlation,
    }


def main() -> int:
    args = parse_args()
    if args.progress_every <= 0 or args.batch_progress_every <= 0:
        print("progress intervals must be positive", file=sys.stderr)
        return 2
    csv_mode = args.dataset is not None
    cache_mode = args.train_cache is not None or args.validation_cache is not None
    if csv_mode == cache_mode:
        print("provide either --dataset or both --train-cache and --validation-cache", file=sys.stderr)
        return 2
    if cache_mode and (args.train_cache is None or args.validation_cache is None):
        print("cache training requires both --train-cache and --validation-cache", file=sys.stderr)
        return 2
    if args.max_train_rows < 0 or args.max_validation_rows < 0:
        print("row caps must not be negative", file=sys.stderr)
        return 2
    if args.target_scale <= 0.0:
        print("--target-scale must be positive", file=sys.stderr)
        return 2
    if args.shaped_weight < 0.0:
        print("--shaped-weight must not be negative", file=sys.stderr)
        return 2

    random.seed(args.seed)
    torch.manual_seed(args.seed)

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    if csv_mode:
        rows = load_rows(args.dataset, args.clip_cp)
        if len(rows) < 2:
            print("dataset needs at least two rows", file=sys.stderr)
            return 1
        random.shuffle(rows)

        validation_count = max(1, int(len(rows) * args.validation_split))
        validation_rows = rows[:validation_count]
        train_rows = rows[validation_count:]
        if args.max_train_rows > 0:
            train_rows = train_rows[: args.max_train_rows]
        if args.max_validation_rows > 0:
            validation_rows = validation_rows[: args.max_validation_rows]
        print("[train] encoding train positions", file=sys.stderr, flush=True)
        train_dataset: Dataset[Any] = NnueDataset(train_rows, args.progress_every)
        print("[train] encoding validation positions", file=sys.stderr, flush=True)
        validation_dataset: Dataset[Any] = NnueDataset(validation_rows, args.progress_every)
        train_loader = DataLoader(
            train_dataset,
            batch_size=args.batch_size,
            shuffle=True,
            collate_fn=lambda batch: collate_csv(batch, device),
        )
        validation_loader = DataLoader(
            validation_dataset,
            batch_size=args.batch_size,
            shuffle=False,
            collate_fn=lambda batch: collate_csv(batch, device),
        )
        dataset_metadata: dict[str, object] = {
            "dataset": str(args.dataset),
            "rows": len(rows),
            "validation_split": args.validation_split,
        }
    else:
        train_dataset = CachedNnueDataset(args.train_cache, args.max_train_rows)
        validation_dataset = CachedNnueDataset(args.validation_cache, args.max_validation_rows)
        train_loader = DataLoader(
            train_dataset,
            batch_size=args.batch_size,
            shuffle=True,
            collate_fn=lambda batch: collate_cache(batch, device),
        )
        validation_loader = DataLoader(
            validation_dataset,
            batch_size=args.batch_size,
            shuffle=False,
            collate_fn=lambda batch: collate_cache(batch, device),
        )
        dataset_metadata = {
            "train_cache": str(args.train_cache),
            "validation_cache": str(args.validation_cache),
            "train_rows": len(train_dataset),
            "validation_rows": len(validation_dataset),
        }

    print(
        f"[train] device={device} train={len(train_dataset)} validation={len(validation_dataset)} "
        f"hidden={args.hidden_size} batch={args.batch_size} epochs={args.epochs} "
        f"loss_mode={args.loss_mode} target_scale={args.target_scale:g} shaped_weight={args.shaped_weight:g}",
        flush=True,
    )
    if device.type == "cuda":
        print(f"[train] cuda_device={torch.cuda.get_device_name(0)}", flush=True)

    model = NnueModel(hidden_size=args.hidden_size).to(device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=args.learning_rate)
    loss_fn = nn.SmoothL1Loss()

    for epoch in range(1, args.epochs + 1):
        epoch_start = time.monotonic()
        model.train()
        total_loss = 0.0
        total = 0
        batch_count = len(train_loader)
        for batch_index, batch in enumerate(train_loader, start=1):
            optimizer.zero_grad(set_to_none=True)
            prediction = model(batch.white, batch.white_mask, batch.black, batch.black_mask, batch.side_to_move)
            loss = objective_loss(
                prediction,
                batch.target,
                loss_fn,
                args.loss_mode,
                args.target_scale,
                args.shaped_weight,
            )
            loss.backward()
            optimizer.step()
            batch_size = int(batch.target.numel())
            total_loss += float(loss.detach()) * batch_size
            total += batch_size
            if batch_index % args.batch_progress_every == 0 or batch_index == batch_count:
                print(
                    f"[train] epoch={epoch}/{args.epochs} batch={batch_index}/{batch_count} "
                    f"rows={total}/{len(train_dataset)} loss={total_loss / max(1, total):.3f}",
                    file=sys.stderr,
                    flush=True,
                )
        train_loss = total_loss / max(1, total)
        validation = evaluate(
            model,
            validation_loader,
            device,
            args.loss_mode,
            args.target_scale,
            args.shaped_weight,
        )
        elapsed = time.monotonic() - epoch_start
        print(
            f"epoch={epoch} train_loss={train_loss:.3f} validation_objective={validation['objective']:.3f} "
            f"validation_raw_loss={validation['raw_loss']:.3f} "
            f"validation_mae={validation['mae']:.1f} validation_rmse={validation['rmse']:.1f} "
            f"validation_corr={validation['correlation']:.3f} elapsed_s={elapsed:.1f}",
            flush=True,
        )

    save_checkpoint(
        args.output,
        model,
        {
            **dataset_metadata,
            "clip_cp": args.clip_cp,
            "device": str(device),
            "loss_mode": args.loss_mode,
            "target_scale": args.target_scale,
            "shaped_weight": args.shaped_weight,
        },
    )
    print(f"wrote checkpoint {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
