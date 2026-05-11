#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import random
import sys
import time
from dataclasses import dataclass
from pathlib import Path

import chess
import torch
from torch import nn
from torch.utils.data import DataLoader, Dataset

from nnue_model import (
    DEFAULT_ACCUMULATOR_SCALE,
    DEFAULT_L2_SIZE,
    DEFAULT_L3_SIZE,
    DEFAULT_OUTPUT_SCALE,
    NnueModel,
    active_features,
    export_binary,
    load_checkpoint,
    make_batch,
)


@dataclass(frozen=True)
class PairItem:
    positive: tuple[list[int], list[int], int, float]
    negative: tuple[list[int], list[int], int, float]
    side_sign: float
    margin_cp: float
    positive_target_cp: float
    negative_target_cp: float


@dataclass(frozen=True)
class PairBatch:
    positive: object
    negative: object
    side_sign: torch.Tensor
    margin_cp: torch.Tensor
    positive_target_cp: torch.Tensor
    negative_target_cp: torch.Tensor


class PairwiseDataset(Dataset):
    def __init__(self, path: Path, feature_set: str, row_limit: int = 0) -> None:
        self.items: list[PairItem] = []
        with path.open(encoding="utf-8", newline="") as handle:
            reader = csv.DictReader(handle)
            for row in reader:
                if row_limit > 0 and len(self.items) >= row_limit:
                    break
                try:
                    parent = chess.Board(row["parent_fen"])
                    positive = chess.Board(row["positive_child_fen"])
                    negative = chess.Board(row["negative_child_fen"])
                    side_sign = float(row.get("side_sign") or (1 if parent.turn == chess.WHITE else -1))
                    margin_cp = float(row["margin_cp"])
                    positive_target = float(row.get("positive_target_cp") or (side_sign * margin_cp / 2.0))
                    negative_target = float(row.get("negative_target_cp") or (-side_sign * margin_cp / 2.0))
                except (KeyError, ValueError):
                    continue
                self.items.append(
                    PairItem(
                        positive=(
                            active_features(positive, chess.WHITE, feature_set),
                            active_features(positive, chess.BLACK, feature_set),
                            1 if positive.turn == chess.WHITE else -1,
                            positive_target,
                        ),
                        negative=(
                            active_features(negative, chess.WHITE, feature_set),
                            active_features(negative, chess.BLACK, feature_set),
                            1 if negative.turn == chess.WHITE else -1,
                            negative_target,
                        ),
                        side_sign=side_sign,
                        margin_cp=margin_cp,
                        positive_target_cp=positive_target,
                        negative_target_cp=negative_target,
                    )
                )
        if not self.items:
            raise ValueError(f"no usable pair rows in {path}")
        print(f"[ranker] loaded pairs={path} rows={len(self.items)}", file=sys.stderr, flush=True)

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, index: int) -> PairItem:
        return self.items[index]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Fine-tune an NNUE checkpoint with pairwise move-ranking supervision.")
    parser.add_argument("--checkpoint", required=True, type=Path, help="Base checkpoint to fine-tune, usually v9 best.pt/current.pt.")
    parser.add_argument("--pairs", required=True, type=Path, help="Pairwise train CSV from build_nnue_pairwise_dataset.py.")
    parser.add_argument("--validation-pairs", type=Path, help="Optional pairwise validation CSV.")
    parser.add_argument("--output", required=True, type=Path, help="Output fine-tuned PyTorch checkpoint.")
    parser.add_argument("--export", type=Path, help="Optional exported .nnue path.")
    parser.add_argument("--epochs", type=int, default=3)
    parser.add_argument("--batch-size", type=int, default=128)
    parser.add_argument("--learning-rate", type=float, default=3e-5)
    parser.add_argument("--ranking-weight", type=float, default=1.0)
    parser.add_argument("--value-weight", type=float, default=0.05)
    parser.add_argument("--margin-scale", type=float, default=1.0, help="Multiply CSV margins by this value.")
    parser.add_argument("--max-train-rows", type=int, default=0)
    parser.add_argument("--max-validation-rows", type=int, default=0)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--progress-every", type=int, default=25)
    return parser.parse_args()


def collate_pairs(items: list[PairItem], device: torch.device) -> PairBatch:
    positive = make_batch([item.positive for item in items], device)
    negative = make_batch([item.negative for item in items], device)
    return PairBatch(
        positive=positive,
        negative=negative,
        side_sign=torch.tensor([item.side_sign for item in items], device=device, dtype=torch.float32),
        margin_cp=torch.tensor([item.margin_cp for item in items], device=device, dtype=torch.float32),
        positive_target_cp=torch.tensor([item.positive_target_cp for item in items], device=device, dtype=torch.float32),
        negative_target_cp=torch.tensor([item.negative_target_cp for item in items], device=device, dtype=torch.float32),
    )


def predict(model: NnueModel, batch: object) -> torch.Tensor:
    return model(
        batch.white,
        batch.white_mask,
        batch.black,
        batch.black_mask,
        batch.side_to_move,
        batch.white_threat,
        batch.white_threat_mask,
        batch.black_threat,
        batch.black_threat_mask,
    )


def pair_losses(
    model: NnueModel,
    batch: PairBatch,
    args: argparse.Namespace,
    value_loss_fn: nn.SmoothL1Loss,
) -> tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]:
    positive = predict(model, batch.positive)
    negative = predict(model, batch.negative)
    signed_gap = (positive - negative) * batch.side_sign
    margin = batch.margin_cp * args.margin_scale
    ranking_loss = torch.relu(margin - signed_gap).mean()
    value_loss = (value_loss_fn(positive, batch.positive_target_cp) + value_loss_fn(negative, batch.negative_target_cp)) * 0.5
    total = args.ranking_weight * ranking_loss + args.value_weight * value_loss
    accuracy = (signed_gap >= margin).to(torch.float32).mean()
    return total, ranking_loss.detach(), value_loss.detach(), accuracy.detach()


def run_epoch(
    model: NnueModel,
    loader: DataLoader,
    device: torch.device,
    args: argparse.Namespace,
    optimizer: torch.optim.Optimizer | None,
    epoch: int,
) -> dict[str, float]:
    value_loss_fn = nn.SmoothL1Loss()
    training = optimizer is not None
    model.train(training)
    total_loss = 0.0
    total_ranking = 0.0
    total_value = 0.0
    total_accuracy = 0.0
    total = 0
    for index, batch in enumerate(loader, start=1):
        if training:
            optimizer.zero_grad(set_to_none=True)
        with torch.set_grad_enabled(training):
            loss, ranking, value, accuracy = pair_losses(model, batch, args, value_loss_fn)
            if training:
                loss.backward()
                optimizer.step()
        batch_size = int(batch.margin_cp.numel())
        total_loss += float(loss.detach()) * batch_size
        total_ranking += float(ranking) * batch_size
        total_value += float(value) * batch_size
        total_accuracy += float(accuracy) * batch_size
        total += batch_size
        if training and (index % args.progress_every == 0 or index == len(loader)):
            print(
                f"[ranker] epoch={epoch}/{args.epochs} batch={index}/{len(loader)} "
                f"pairs={total}/{len(loader.dataset)} loss={total_loss / max(1, total):.3f}",
                file=sys.stderr,
                flush=True,
            )
    total = max(1, total)
    return {
        "loss": total_loss / total,
        "ranking_loss": total_ranking / total,
        "value_loss": total_value / total,
        "pair_accuracy": total_accuracy / total,
    }


def checkpoint_payload(model: NnueModel, metadata: dict[str, object], args: argparse.Namespace) -> dict[str, object]:
    return {
        "hidden_size": model.hidden_size,
        "format_version": model.format_version,
        "feature_set": model.feature_set,
        "feature_count": model.feature_count,
        "placement_feature_count": model.placement_feature_count,
        "threat_feature_count": model.threat_feature_count,
        "perspective_mode": model.perspective_mode,
        "architecture": model.architecture,
        "l2_size": model.l2_size,
        "l3_size": model.l3_size,
        "state_dict": model.state_dict(),
        "metadata": metadata,
        "ranking_config": {
            "base_checkpoint": str(args.checkpoint),
            "pairs": str(args.pairs),
            "validation_pairs": str(args.validation_pairs) if args.validation_pairs else None,
            "epochs": args.epochs,
            "batch_size": args.batch_size,
            "learning_rate": args.learning_rate,
            "ranking_weight": args.ranking_weight,
            "value_weight": args.value_weight,
            "margin_scale": args.margin_scale,
            "seed": args.seed,
        },
    }


def main() -> int:
    args = parse_args()
    if args.epochs <= 0 or args.batch_size <= 0 or args.progress_every <= 0:
        print("epochs, batch size, and progress interval must be positive", file=sys.stderr)
        return 2
    if args.learning_rate <= 0.0 or args.ranking_weight < 0.0 or args.value_weight < 0.0 or args.margin_scale <= 0.0:
        print("learning rate/weights/margin scale are invalid", file=sys.stderr)
        return 2
    if args.max_train_rows < 0 or args.max_validation_rows < 0:
        print("row limits must not be negative", file=sys.stderr)
        return 2

    random.seed(args.seed)
    torch.manual_seed(args.seed)
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model, metadata = load_checkpoint(args.checkpoint, device)
    metadata = {
        **metadata,
        "ranker_base_checkpoint": str(args.checkpoint),
        "ranker_pairs": str(args.pairs),
        "ranker_validation_pairs": str(args.validation_pairs) if args.validation_pairs else None,
        "ranker_loss": "margin-ranking+value" if args.value_weight > 0.0 else "margin-ranking",
    }

    train_dataset = PairwiseDataset(args.pairs, model.feature_set, args.max_train_rows)
    train_loader = DataLoader(
        train_dataset,
        batch_size=args.batch_size,
        shuffle=True,
        collate_fn=lambda items: collate_pairs(items, device),
    )
    validation_loader = None
    if args.validation_pairs is not None:
        validation_dataset = PairwiseDataset(args.validation_pairs, model.feature_set, args.max_validation_rows)
        validation_loader = DataLoader(
            validation_dataset,
            batch_size=args.batch_size,
            shuffle=False,
            collate_fn=lambda items: collate_pairs(items, device),
        )

    optimizer = torch.optim.AdamW(model.parameters(), lr=args.learning_rate)
    best_validation = float("inf")
    best_state = None
    started = time.monotonic()
    for epoch in range(1, args.epochs + 1):
        train_metrics = run_epoch(model, train_loader, device, args, optimizer, epoch)
        message = (
            f"epoch={epoch} train_loss={train_metrics['loss']:.3f} "
            f"train_ranking_loss={train_metrics['ranking_loss']:.3f} "
            f"train_value_loss={train_metrics['value_loss']:.3f} "
            f"train_pair_accuracy={train_metrics['pair_accuracy']:.3f}"
        )
        if validation_loader is not None:
            validation_metrics = run_epoch(model, validation_loader, device, args, None, epoch)
            message += (
                f" validation_loss={validation_metrics['loss']:.3f} "
                f"validation_pair_accuracy={validation_metrics['pair_accuracy']:.3f}"
            )
            if validation_metrics["loss"] < best_validation:
                best_validation = validation_metrics["loss"]
                best_state = {key: value.detach().cpu().clone() for key, value in model.state_dict().items()}
        print(message, flush=True)

    if best_state is not None:
        model.load_state_dict(best_state)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    torch.save(checkpoint_payload(model, metadata, args), args.output)
    print(f"wrote rank-tuned checkpoint {args.output} elapsed_s={time.monotonic() - started:.1f}")
    if args.export is not None:
        args.export.parent.mkdir(parents=True, exist_ok=True)
        export_binary(args.export, model.cpu(), DEFAULT_ACCUMULATOR_SCALE, DEFAULT_OUTPUT_SCALE)
        print(f"wrote exported rank-tuned NNUE {args.export}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
