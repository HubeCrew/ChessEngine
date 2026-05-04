#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import random
import sys
import time
from pathlib import Path

import chess
import torch
from torch import nn
from torch.utils.data import DataLoader, Dataset

from nnue_model import NnueModel, active_features, make_batch, save_checkpoint


class NnueDataset(Dataset):
    def __init__(self, rows: list[tuple[str, float]], progress_every: int) -> None:
        self.items: list[tuple[list[int], list[int], float]] = []
        for index, (fen, score) in enumerate(rows, start=1):
            board = chess.Board(fen)
            self.items.append((active_features(board, chess.WHITE), active_features(board, chess.BLACK), score))
            if index % progress_every == 0 or index == len(rows):
                print(f"[train] encoded={index}/{len(rows)}", file=sys.stderr, flush=True)

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, index: int) -> tuple[list[int], list[int], float]:
        return self.items[index]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train a small HalfKP-style NNUE model from labeled CSV positions.")
    parser.add_argument("--dataset", required=True, type=Path, help="CSV with fen and score_cp columns.")
    parser.add_argument("--output", required=True, type=Path, help="Output PyTorch checkpoint.")
    parser.add_argument("--hidden-size", type=int, default=256)
    parser.add_argument("--epochs", type=int, default=5)
    parser.add_argument("--batch-size", type=int, default=512)
    parser.add_argument("--learning-rate", type=float, default=0.001)
    parser.add_argument("--validation-split", type=float, default=0.05)
    parser.add_argument("--clip-cp", type=int, default=1500)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--progress-every", type=int, default=5000, help="Feature-encoding progress interval.")
    parser.add_argument("--batch-progress-every", type=int, default=25, help="Training-batch progress interval.")
    return parser.parse_args()


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


def evaluate(model: NnueModel, loader: DataLoader, device: torch.device) -> dict[str, float]:
    loss_fn = nn.SmoothL1Loss()
    total_loss = 0.0
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
        for items in loader:
            batch = make_batch(list(items), device)
            prediction = model(batch.white, batch.white_mask, batch.black, batch.black_mask)
            loss = loss_fn(prediction, batch.target)
            total_loss += float(loss) * len(items)
            diff = prediction - batch.target
            total_abs += float(diff.abs().sum())
            total_squared += float((diff * diff).sum())
            sum_prediction += float(prediction.sum())
            sum_target += float(batch.target.sum())
            sum_prediction_squared += float((prediction * prediction).sum())
            sum_target_squared += float((batch.target * batch.target).sum())
            sum_product += float((prediction * batch.target).sum())
            total += len(items)

    total = max(1, total)
    covariance = sum_product - (sum_prediction * sum_target / total)
    prediction_variance = sum_prediction_squared - (sum_prediction * sum_prediction / total)
    target_variance = sum_target_squared - (sum_target * sum_target / total)
    if prediction_variance <= 0.0 or target_variance <= 0.0:
        correlation = 0.0
    else:
        correlation = covariance / ((prediction_variance * target_variance) ** 0.5)
    return {
        "loss": total_loss / total,
        "mae": total_abs / total,
        "rmse": (total_squared / total) ** 0.5,
        "correlation": correlation,
    }


def main() -> int:
    args = parse_args()
    if args.progress_every <= 0 or args.batch_progress_every <= 0:
        print("progress intervals must be positive", file=sys.stderr)
        return 2

    random.seed(args.seed)
    torch.manual_seed(args.seed)

    rows = load_rows(args.dataset, args.clip_cp)
    if len(rows) < 2:
        print("dataset needs at least two rows", file=sys.stderr)
        return 1
    random.shuffle(rows)

    validation_count = max(1, int(len(rows) * args.validation_split))
    validation_rows = rows[:validation_count]
    train_rows = rows[validation_count:]

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(
        f"[train] device={device} train={len(train_rows)} validation={len(validation_rows)} "
        f"hidden={args.hidden_size} batch={args.batch_size} epochs={args.epochs}",
        flush=True,
    )
    if device.type == "cuda":
        print(f"[train] cuda_device={torch.cuda.get_device_name(0)}", flush=True)

    print("[train] encoding train positions", file=sys.stderr, flush=True)
    train_dataset = NnueDataset(train_rows, args.progress_every)
    print("[train] encoding validation positions", file=sys.stderr, flush=True)
    validation_dataset = NnueDataset(validation_rows, args.progress_every)
    train_loader = DataLoader(train_dataset, batch_size=args.batch_size, shuffle=True, collate_fn=lambda batch: batch)
    validation_loader = DataLoader(validation_dataset, batch_size=args.batch_size, shuffle=False, collate_fn=lambda batch: batch)

    model = NnueModel(hidden_size=args.hidden_size).to(device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=args.learning_rate)
    loss_fn = nn.SmoothL1Loss()

    for epoch in range(1, args.epochs + 1):
        epoch_start = time.monotonic()
        model.train()
        total_loss = 0.0
        total = 0
        batch_count = len(train_loader)
        for batch_index, items in enumerate(train_loader, start=1):
            batch = make_batch(list(items), device)
            optimizer.zero_grad(set_to_none=True)
            prediction = model(batch.white, batch.white_mask, batch.black, batch.black_mask)
            loss = loss_fn(prediction, batch.target)
            loss.backward()
            optimizer.step()
            total_loss += float(loss.detach()) * len(items)
            total += len(items)
            if batch_index % args.batch_progress_every == 0 or batch_index == batch_count:
                print(
                    f"[train] epoch={epoch}/{args.epochs} batch={batch_index}/{batch_count} "
                    f"rows={total}/{len(train_dataset)} loss={total_loss / max(1, total):.3f}",
                    file=sys.stderr,
                    flush=True,
                )
        train_loss = total_loss / max(1, total)
        validation = evaluate(model, validation_loader, device)
        elapsed = time.monotonic() - epoch_start
        print(
            f"epoch={epoch} train_loss={train_loss:.3f} validation_loss={validation['loss']:.3f} "
            f"validation_mae={validation['mae']:.1f} validation_rmse={validation['rmse']:.1f} "
            f"validation_corr={validation['correlation']:.3f} elapsed_s={elapsed:.1f}",
            flush=True,
        )

    save_checkpoint(
        args.output,
        model,
        {
            "dataset": str(args.dataset),
            "rows": len(rows),
            "clip_cp": args.clip_cp,
            "device": str(device),
        },
    )
    print(f"wrote checkpoint {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
