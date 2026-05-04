#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import random
import sys
from pathlib import Path

import chess
import torch
from torch import nn
from torch.utils.data import DataLoader, Dataset

from nnue_model import NnueModel, active_features, make_batch, save_checkpoint


class NnueDataset(Dataset):
    def __init__(self, rows: list[tuple[str, float]]) -> None:
        self.items: list[tuple[list[int], list[int], float]] = []
        for fen, score in rows:
            board = chess.Board(fen)
            self.items.append((active_features(board, chess.WHITE), active_features(board, chess.BLACK), score))

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
    return parser.parse_args()


def load_rows(path: Path, clip_cp: int) -> list[tuple[str, float]]:
    rows: list[tuple[str, float]] = []
    with path.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            fen = row["fen"]
            score = max(-clip_cp, min(clip_cp, float(row["score_cp"])))
            rows.append((fen, score))
    return rows


def evaluate(model: NnueModel, loader: DataLoader, device: torch.device) -> float:
    loss_fn = nn.SmoothL1Loss()
    total_loss = 0.0
    total = 0
    model.eval()
    with torch.no_grad():
        for items in loader:
            batch = make_batch(list(items), device)
            prediction = model(batch.white, batch.white_mask, batch.black, batch.black_mask)
            loss = loss_fn(prediction, batch.target)
            total_loss += float(loss) * len(items)
            total += len(items)
    return total_loss / max(1, total)


def main() -> int:
    args = parse_args()
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
    print(f"device={device} train={len(train_rows)} validation={len(validation_rows)}")

    train_dataset = NnueDataset(train_rows)
    validation_dataset = NnueDataset(validation_rows)
    train_loader = DataLoader(train_dataset, batch_size=args.batch_size, shuffle=True, collate_fn=lambda batch: batch)
    validation_loader = DataLoader(validation_dataset, batch_size=args.batch_size, shuffle=False, collate_fn=lambda batch: batch)

    model = NnueModel(hidden_size=args.hidden_size).to(device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=args.learning_rate)
    loss_fn = nn.SmoothL1Loss()

    for epoch in range(1, args.epochs + 1):
        model.train()
        total_loss = 0.0
        total = 0
        for items in train_loader:
            batch = make_batch(list(items), device)
            optimizer.zero_grad(set_to_none=True)
            prediction = model(batch.white, batch.white_mask, batch.black, batch.black_mask)
            loss = loss_fn(prediction, batch.target)
            loss.backward()
            optimizer.step()
            total_loss += float(loss.detach()) * len(items)
            total += len(items)
        train_loss = total_loss / max(1, total)
        validation_loss = evaluate(model, validation_loader, device)
        print(f"epoch={epoch} train_loss={train_loss:.3f} validation_loss={validation_loss:.3f}")

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
