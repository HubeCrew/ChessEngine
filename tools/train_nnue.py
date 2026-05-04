#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import csv
import shutil
import random
import re
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

import chess
import torch
from torch import nn
from torch.utils.data import DataLoader, Dataset

from nnue_model import (
    DEFAULT_ACCUMULATOR_SCALE,
    DEFAULT_L2_SIZE,
    DEFAULT_L3_SIZE,
    DEFAULT_OUTPUT_SCALE,
    FEATURE_SET_HALFKP,
    SUPPORTED_FEATURE_SETS,
    Batch,
    NnueModel,
    active_features,
    export_binary,
    feature_count_for,
    make_batch,
    save_checkpoint,
)


class NnueDataset(Dataset):
    def __init__(self, rows: list[tuple[str, float]], progress_every: int, feature_set: str) -> None:
        self.items: list[tuple[list[int], list[int], int, float]] = []
        self.feature_set = feature_set
        self.feature_count = feature_count_for(feature_set)
        for index, (fen, score) in enumerate(rows, start=1):
            board = chess.Board(fen)
            side_to_move = 1 if board.turn == chess.WHITE else -1
            self.items.append(
                (
                    active_features(board, chess.WHITE, feature_set),
                    active_features(board, chess.BLACK, feature_set),
                    side_to_move,
                    score,
                )
            )
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
        format_version = int(self.cache.get("format_version", 0))
        if format_version not in (2, 3):
            raise ValueError(f"unsupported feature cache format in {path}")
        self.feature_set = str(self.cache.get("feature_set", FEATURE_SET_HALFKP))
        if self.feature_set not in SUPPORTED_FEATURE_SETS:
            raise ValueError(f"unsupported feature cache feature_set {self.feature_set} in {path}")
        self.feature_count = int(self.cache.get("feature_count", feature_count_for(self.feature_set)))
        expected_feature_count = feature_count_for(self.feature_set)
        if self.feature_count != expected_feature_count:
            raise ValueError(
                f"feature cache {path} feature_count={self.feature_count} does not match "
                f"{self.feature_set} expected={expected_feature_count}"
            )
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
        print(
            f"[train] loaded feature_cache={path} rows={rows} "
            f"feature_set={self.feature_set} feature_count={self.feature_count}",
            file=sys.stderr,
            flush=True,
        )

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
    parser.add_argument("--resume", type=Path, help="Resume from a run checkpoint, usually <run-dir>/last.pt.")
    parser.add_argument("--dataset", type=Path, help="CSV with fen and score_cp columns. Uses --validation-split.")
    parser.add_argument("--train-cache", type=Path, help="Feature cache built by build_nnue_feature_cache.py.")
    parser.add_argument("--validation-cache", type=Path, help="Validation feature cache built by build_nnue_feature_cache.py.")
    parser.add_argument("--output", type=Path, help="Output PyTorch checkpoint for legacy final-only training.")
    parser.add_argument("--run-dir", type=Path, help="Run directory for resumable training with last/best checkpoints and metrics.")
    parser.add_argument("--hidden-size", type=int, default=256)
    parser.add_argument(
        "--feature-set",
        choices=SUPPORTED_FEATURE_SETS,
        help="NNUE feature mapping. Cache training infers this from caches unless explicitly provided.",
    )
    parser.add_argument(
        "--architecture",
        choices=("sf-lite", "linear"),
        default="sf-lite",
        help="sf-lite uses a Stockfish-inspired dense stack. linear preserves the older one-layer NNUE.",
    )
    parser.add_argument("--l2-size", type=int, default=DEFAULT_L2_SIZE, help="SF-lite first dense layer width.")
    parser.add_argument("--l3-size", type=int, default=DEFAULT_L3_SIZE, help="SF-lite second dense layer width.")
    parser.add_argument(
        "--perspective-mode",
        choices=("side-to-move", "fixed"),
        default="side-to-move",
        help="side-to-move trains Stockfish-style us/them accumulator order. fixed preserves the older white/black order.",
    )
    parser.add_argument("--epochs", type=int, default=5)
    parser.add_argument("--batch-size", type=int, default=512)
    parser.add_argument("--learning-rate", type=float, default=0.001)
    parser.add_argument("--lr-patience", type=int, default=2)
    parser.add_argument("--lr-factor", type=float, default=0.5)
    parser.add_argument("--min-learning-rate", type=float, default=1e-6)
    parser.add_argument("--early-stop-patience", type=int, default=0, help="0 disables early stopping.")
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
    parser.add_argument("--best-metric", choices=("validation_mae",), default="validation_mae")
    parser.add_argument("--save-every", type=int, default=0, help="Save epoch_XXXX.pt every N epochs. 0 disables archival snapshots.")
    parser.add_argument("--holdout-dataset", type=Path, help="Optional holdout CSV evaluated after training against best.pt.")
    parser.add_argument("--export-best", action="store_true", help="Export best.pt to best.nnue after training.")
    parser.add_argument("--parity-engine", type=Path, help="Engine executable for post-export parity check.")
    parser.add_argument("--parity-limit", type=int, default=256)
    parser.add_argument("--engine-tolerance", type=int, default=0)
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


def provided_options(argv: list[str]) -> set[str]:
    provided: set[str] = set()
    for token in argv:
        if token.startswith("--"):
            provided.add(token[2:].split("=", 1)[0].replace("-", "_"))
    return provided


def json_ready(value: object) -> object:
    if isinstance(value, Path):
        return str(value)
    if isinstance(value, dict):
        return {str(key): json_ready(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [json_ready(item) for item in value]
    return value


def checkpoint_config(args: argparse.Namespace, dataset_metadata: dict[str, object]) -> dict[str, object]:
    keys = (
        "dataset",
        "train_cache",
        "validation_cache",
        "epochs",
        "hidden_size",
        "feature_set",
        "architecture",
        "l2_size",
        "l3_size",
        "perspective_mode",
        "batch_size",
        "learning_rate",
        "lr_patience",
        "lr_factor",
        "min_learning_rate",
        "loss_mode",
        "target_scale",
        "shaped_weight",
        "validation_split",
        "clip_cp",
        "seed",
        "max_train_rows",
        "max_validation_rows",
        "progress_every",
        "batch_progress_every",
        "best_metric",
        "save_every",
    )
    config = {key: json_ready(getattr(args, key)) for key in keys}
    config["dataset_metadata"] = json_ready(dataset_metadata)
    return config


def apply_resume_config(args: argparse.Namespace, checkpoint: dict[str, object], provided: set[str]) -> None:
    config = dict(checkpoint.get("training_config", {}))
    for key, value in config.items():
        if key == "dataset_metadata" or key in provided:
            continue
        if key in ("dataset", "train_cache", "validation_cache") and value is not None:
            setattr(args, key, Path(str(value)))
        elif hasattr(args, key):
            setattr(args, key, value)


def validate_resume_compatibility(args: argparse.Namespace, checkpoint: dict[str, object]) -> None:
    model_shape = {
        "hidden_size": int(checkpoint["hidden_size"]),
        "architecture": str(checkpoint.get("architecture", "linear")),
        "l2_size": int(checkpoint.get("l2_size", DEFAULT_L2_SIZE)),
        "l3_size": int(checkpoint.get("l3_size", DEFAULT_L3_SIZE)),
        "perspective_mode": str(checkpoint.get("perspective_mode", "fixed")),
        "feature_set": str(checkpoint.get("feature_set", FEATURE_SET_HALFKP)),
    }
    requested = {
        "hidden_size": int(args.hidden_size),
        "architecture": str(args.architecture),
        "l2_size": int(args.l2_size),
        "l3_size": int(args.l3_size),
        "perspective_mode": str(args.perspective_mode),
        "feature_set": str(args.feature_set),
    }
    if model_shape != requested:
        raise ValueError(f"resume checkpoint shape {model_shape} does not match requested shape {requested}")


def training_checkpoint(
    model: NnueModel,
    optimizer: torch.optim.Optimizer,
    scheduler: torch.optim.lr_scheduler.ReduceLROnPlateau,
    metadata: dict[str, object],
    training_config: dict[str, object],
    epoch: int,
    best_metric: float,
    best_epoch: int,
) -> dict[str, object]:
    return {
        "hidden_size": model.hidden_size,
        "format_version": model.format_version,
        "feature_set": model.feature_set,
        "feature_count": model.feature_count,
        "perspective_mode": model.perspective_mode,
        "architecture": model.architecture,
        "l2_size": model.l2_size,
        "l3_size": model.l3_size,
        "state_dict": model.state_dict(),
        "metadata": metadata,
        "training_config": training_config,
        "optimizer_state": optimizer.state_dict(),
        "scheduler_state": scheduler.state_dict(),
        "completed_epoch": epoch,
        "best_metric": best_metric,
        "best_epoch": best_epoch,
        "rng_state": torch.get_rng_state(),
        "cuda_rng_state": torch.cuda.get_rng_state_all() if torch.cuda.is_available() else None,
        "python_random_state": random.getstate(),
    }


def save_training_checkpoint(path: Path, payload: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    torch.save(payload, path)


def append_metrics(run_dir: Path, row: dict[str, object]) -> None:
    csv_path = run_dir / "metrics.csv"
    jsonl_path = run_dir / "metrics.jsonl"
    fieldnames = [
        "epoch",
        "train_loss",
        "validation_objective",
        "validation_raw_loss",
        "validation_mae",
        "validation_rmse",
        "validation_corr",
        "learning_rate",
        "elapsed_s",
        "is_best",
    ]
    write_header = not csv_path.exists()
    with csv_path.open("a", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        if write_header:
            writer.writeheader()
        writer.writerow({key: row.get(key, "") for key in fieldnames})
    with jsonl_path.open("a", encoding="utf-8") as handle:
        handle.write(json.dumps(json_ready(row), sort_keys=True) + "\n")


def parse_holdout_metrics(report: str) -> dict[str, dict[str, float | int]]:
    pattern = re.compile(
        r"^(?P<name>\S(?:.*?\S)?)\s+count=\s*(?P<count>\d+)\s+mae=\s*(?P<mae>-?\d+(?:\.\d+)?)\s+"
        r"rmse=\s*(?P<rmse>-?\d+(?:\.\d+)?)\s+bias=\s*(?P<bias>-?\d+(?:\.\d+)?)\s+"
        r"corr=\s*(?P<corr>-?\d+(?:\.\d+)?)$"
    )
    metrics: dict[str, dict[str, float | int]] = {}
    for line in report.splitlines():
        match = pattern.match(line.strip())
        if match is None:
            continue
        metrics[match.group("name")] = {
            "count": int(match.group("count")),
            "mae": float(match.group("mae")),
            "rmse": float(match.group("rmse")),
            "bias": float(match.group("bias")),
            "corr": float(match.group("corr")),
        }
    return metrics


def evaluate_holdout_report(checkpoint: Path, dataset: Path, batch_size: int, output: Path, metrics_output: Path) -> int:
    command = [
        sys.executable,
        str(Path(__file__).with_name("evaluate_nnue_checkpoint.py")),
        "--checkpoint",
        str(checkpoint),
        "--dataset",
        str(dataset),
        "--batch-size",
        str(batch_size),
    ]
    completed = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
    output.write_text(completed.stdout, encoding="utf-8")
    metrics_output.write_text(json.dumps(parse_holdout_metrics(completed.stdout), indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(completed.stdout, end="")
    return completed.returncode


def export_best_checkpoint(checkpoint: Path, output: Path) -> None:
    device = torch.device("cpu")
    loaded = torch.load(checkpoint, map_location=device)
    model = NnueModel(
        hidden_size=int(loaded["hidden_size"]),
        perspective_mode=str(loaded.get("perspective_mode", "fixed")),
        architecture=str(loaded.get("architecture", "linear")),
        l2_size=int(loaded.get("l2_size", DEFAULT_L2_SIZE)),
        l3_size=int(loaded.get("l3_size", DEFAULT_L3_SIZE)),
        feature_set=str(loaded.get("feature_set", FEATURE_SET_HALFKP)),
    )
    model.load_state_dict(loaded["state_dict"])
    model.eval()
    export_binary(output, model, DEFAULT_ACCUMULATOR_SCALE, DEFAULT_OUTPUT_SCALE)


def run_parity_report(checkpoint: Path, nnue: Path, engine: Path, dataset: Path, limit: int, tolerance: int, output: Path) -> int:
    command = [
        sys.executable,
        str(Path(__file__).with_name("check_nnue_parity.py")),
        "--checkpoint",
        str(checkpoint),
        "--nnue",
        str(nnue),
        "--engine",
        str(engine),
        "--dataset",
        str(dataset),
        "--limit",
        str(limit),
        "--engine-tolerance",
        str(tolerance),
    ]
    completed = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
    output.write_text(completed.stdout, encoding="utf-8")
    print(completed.stdout, end="")
    return completed.returncode


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
    provided = provided_options(sys.argv[1:])
    resume_checkpoint: dict[str, object] | None = None
    if args.resume is not None:
        print(f"[train] loading resume checkpoint={args.resume}", file=sys.stderr, flush=True)
        resume_checkpoint = torch.load(args.resume, map_location="cpu", weights_only=False)
        if "training_config" not in resume_checkpoint:
            print("resume checkpoint does not contain training_config", file=sys.stderr)
            return 2
        apply_resume_config(args, resume_checkpoint, provided)
        if args.run_dir is None:
            args.run_dir = args.resume.parent
    if args.output is None and args.run_dir is None:
        print("provide --output for legacy training or --run-dir for managed training", file=sys.stderr)
        return 2
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
    if args.architecture == "sf-lite" and args.perspective_mode != "side-to-move":
        print("sf-lite requires --perspective-mode side-to-move", file=sys.stderr)
        return 2
    if args.architecture != "sf-lite" and args.feature_set not in (None, FEATURE_SET_HALFKP):
        print("non-HalfKP feature sets currently require --architecture sf-lite", file=sys.stderr)
        return 2
    if args.l2_size <= 0 or args.l3_size <= 0:
        print("dense layer sizes must be positive", file=sys.stderr)
        return 2
    if args.lr_patience < 0 or args.early_stop_patience < 0 or args.save_every < 0:
        print("patience and save intervals must not be negative", file=sys.stderr)
        return 2
    if args.lr_factor <= 0.0 or args.lr_factor >= 1.0:
        print("--lr-factor must be greater than 0 and less than 1", file=sys.stderr)
        return 2
    if args.min_learning_rate <= 0.0 or args.learning_rate <= 0.0:
        print("learning rates must be positive", file=sys.stderr)
        return 2
    if args.parity_limit <= 0:
        print("--parity-limit must be positive", file=sys.stderr)
        return 2
    if args.parity_engine is not None and args.holdout_dataset is None:
        print("--parity-engine requires --holdout-dataset so parity can sample FENs", file=sys.stderr)
        return 2
    if args.parity_engine is not None and not args.export_best:
        print("--parity-engine requires --export-best", file=sys.stderr)
        return 2
    if resume_checkpoint is not None and args.feature_set is None:
        args.feature_set = str(resume_checkpoint.get("feature_set", FEATURE_SET_HALFKP))
    if resume_checkpoint is not None:
        try:
            validate_resume_compatibility(args, resume_checkpoint)
        except ValueError as exc:
            print(str(exc), file=sys.stderr)
            return 2

    random.seed(args.seed)
    torch.manual_seed(args.seed)

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    if csv_mode:
        if args.feature_set is None:
            args.feature_set = FEATURE_SET_HALFKP
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
        train_dataset: Dataset[Any] = NnueDataset(train_rows, args.progress_every, args.feature_set)
        print("[train] encoding validation positions", file=sys.stderr, flush=True)
        validation_dataset: Dataset[Any] = NnueDataset(validation_rows, args.progress_every, args.feature_set)
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
            "feature_set": args.feature_set,
            "feature_count": feature_count_for(args.feature_set),
        }
    else:
        train_dataset = CachedNnueDataset(args.train_cache, args.max_train_rows)
        validation_dataset = CachedNnueDataset(args.validation_cache, args.max_validation_rows)
        if train_dataset.feature_set != validation_dataset.feature_set:
            print(
                f"cache feature-set mismatch: train={train_dataset.feature_set} "
                f"validation={validation_dataset.feature_set}",
                file=sys.stderr,
            )
            return 2
        if train_dataset.feature_count != validation_dataset.feature_count:
            print(
                f"cache feature-count mismatch: train={train_dataset.feature_count} "
                f"validation={validation_dataset.feature_count}",
                file=sys.stderr,
            )
            return 2
        if args.feature_set is None:
            args.feature_set = train_dataset.feature_set
        elif args.feature_set != train_dataset.feature_set:
            print(
                f"--feature-set {args.feature_set} does not match cache feature_set {train_dataset.feature_set}",
                file=sys.stderr,
            )
            return 2
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
            "feature_set": args.feature_set,
            "feature_count": feature_count_for(args.feature_set),
        }

    run_dir = args.run_dir
    if run_dir is not None:
        if args.resume is None and (run_dir / "last.pt").exists():
            print(f"run directory already has last.pt; use --resume {run_dir / 'last.pt'} or choose a new --run-dir", file=sys.stderr)
            return 2
        run_dir.mkdir(parents=True, exist_ok=True)

    training_config = checkpoint_config(args, dataset_metadata)
    if run_dir is not None:
        (run_dir / "config.json").write_text(json.dumps(json_ready(training_config), indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(
        f"[train] device={device} train={len(train_dataset)} validation={len(validation_dataset)} "
        f"hidden={args.hidden_size} batch={args.batch_size} epochs={args.epochs} "
        f"architecture={args.architecture} l2={args.l2_size} l3={args.l3_size} "
        f"feature_set={args.feature_set} feature_count={feature_count_for(args.feature_set)} "
        f"perspective_mode={args.perspective_mode} loss_mode={args.loss_mode} "
        f"target_scale={args.target_scale:g} shaped_weight={args.shaped_weight:g}",
        flush=True,
    )
    if device.type == "cuda":
        print(f"[train] cuda_device={torch.cuda.get_device_name(0)}", flush=True)

    model = NnueModel(
        hidden_size=args.hidden_size,
        perspective_mode=args.perspective_mode,
        architecture=args.architecture,
        l2_size=args.l2_size,
        l3_size=args.l3_size,
        feature_set=args.feature_set,
    ).to(device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=args.learning_rate)
    scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(
        optimizer,
        mode="min",
        factor=args.lr_factor,
        patience=args.lr_patience,
        min_lr=args.min_learning_rate,
    )
    loss_fn = nn.SmoothL1Loss()
    start_epoch = 1
    best_metric = float("inf")
    best_epoch = 0
    epochs_without_improvement = 0

    if resume_checkpoint is not None:
        model.load_state_dict(resume_checkpoint["state_dict"])
        optimizer.load_state_dict(resume_checkpoint["optimizer_state"])
        scheduler.load_state_dict(resume_checkpoint["scheduler_state"])
        start_epoch = int(resume_checkpoint.get("completed_epoch", 0)) + 1
        best_metric = float(resume_checkpoint.get("best_metric", float("inf")))
        best_epoch = int(resume_checkpoint.get("best_epoch", 0))
        torch_rng = resume_checkpoint.get("rng_state")
        if isinstance(torch_rng, torch.Tensor):
            torch.set_rng_state(torch_rng)
        cuda_rng = resume_checkpoint.get("cuda_rng_state")
        if cuda_rng is not None and torch.cuda.is_available():
            torch.cuda.set_rng_state_all(cuda_rng)
        python_rng = resume_checkpoint.get("python_random_state")
        if python_rng is not None:
            random.setstate(python_rng)
        print(
            f"[train] resumed completed_epoch={start_epoch - 1} best_epoch={best_epoch} best_validation_mae={best_metric:.3f}",
            flush=True,
        )

    metadata = {
        **dataset_metadata,
        "clip_cp": args.clip_cp,
        "device": str(device),
        "feature_set": args.feature_set,
        "feature_count": feature_count_for(args.feature_set),
        "perspective_mode": args.perspective_mode,
        "architecture": args.architecture,
        "l2_size": args.l2_size,
        "l3_size": args.l3_size,
        "loss_mode": args.loss_mode,
        "target_scale": args.target_scale,
        "shaped_weight": args.shaped_weight,
    }

    if start_epoch > args.epochs:
        print(f"[train] checkpoint already completed epoch {start_epoch - 1}; no additional training requested", flush=True)

    for epoch in range(start_epoch, args.epochs + 1):
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
        metric_value = float(validation["mae"])
        previous_best = best_metric
        is_best = metric_value < best_metric
        if is_best:
            best_metric = metric_value
            best_epoch = epoch
            epochs_without_improvement = 0
        else:
            epochs_without_improvement += 1
        scheduler.step(metric_value)
        learning_rate = float(optimizer.param_groups[0]["lr"])
        print(
            f"epoch={epoch} train_loss={train_loss:.3f} validation_objective={validation['objective']:.3f} "
            f"validation_raw_loss={validation['raw_loss']:.3f} "
            f"validation_mae={validation['mae']:.1f} validation_rmse={validation['rmse']:.1f} "
            f"validation_corr={validation['correlation']:.3f} lr={learning_rate:.6g} "
            f"best_validation_mae={best_metric:.3f} elapsed_s={elapsed:.1f}",
            flush=True,
        )

        if run_dir is not None:
            row = {
                "epoch": epoch,
                "train_loss": train_loss,
                "validation_objective": validation["objective"],
                "validation_raw_loss": validation["raw_loss"],
                "validation_mae": validation["mae"],
                "validation_rmse": validation["rmse"],
                "validation_corr": validation["correlation"],
                "learning_rate": learning_rate,
                "elapsed_s": elapsed,
                "is_best": is_best,
                "previous_best_validation_mae": previous_best,
            }
            append_metrics(run_dir, row)
            payload = training_checkpoint(
                model,
                optimizer,
                scheduler,
                metadata,
                training_config,
                epoch,
                best_metric,
                best_epoch,
            )
            save_training_checkpoint(run_dir / "last.pt", payload)
            if is_best:
                save_training_checkpoint(run_dir / "best.pt", payload)
                print(f"[train] wrote new best checkpoint={run_dir / 'best.pt'} validation_mae={best_metric:.3f}", flush=True)
            if args.save_every > 0 and epoch % args.save_every == 0:
                save_training_checkpoint(run_dir / f"epoch_{epoch:04d}.pt", payload)

        if args.early_stop_patience > 0 and epochs_without_improvement >= args.early_stop_patience:
            print(
                f"[train] early stopping after {epochs_without_improvement} epochs without validation MAE improvement",
                flush=True,
            )
            break

    if args.output is not None:
        save_checkpoint(args.output, model, metadata)
        print(f"wrote checkpoint {args.output}")

    if run_dir is not None and not (run_dir / "best.pt").exists() and (run_dir / "last.pt").exists():
        shutil.copyfile(run_dir / "last.pt", run_dir / "best.pt")

    best_checkpoint = (run_dir / "best.pt") if run_dir is not None else args.output
    if best_checkpoint is None:
        print("no checkpoint available for post-training steps", file=sys.stderr)
        return 1

    if args.holdout_dataset is not None:
        if run_dir is None:
            print("--holdout-dataset requires --run-dir or --resume so reports have a destination", file=sys.stderr)
            return 2
        print(f"[train] evaluating holdout checkpoint={best_checkpoint}", flush=True)
        code = evaluate_holdout_report(
            best_checkpoint,
            args.holdout_dataset,
            args.batch_size,
            run_dir / "holdout_report.txt",
            run_dir / "holdout_metrics.json",
        )
        if code != 0:
            return code

    exported = None
    if args.export_best:
        if run_dir is None:
            print("--export-best requires --run-dir or --resume", file=sys.stderr)
            return 2
        exported = run_dir / "best.nnue"
        print(f"[train] exporting best checkpoint={best_checkpoint} output={exported}", flush=True)
        export_best_checkpoint(best_checkpoint, exported)

    if args.parity_engine is not None:
        if run_dir is None or exported is None or args.holdout_dataset is None:
            print("parity requires --run-dir/--resume, --export-best, and --holdout-dataset", file=sys.stderr)
            return 2
        print(f"[train] running parity engine={args.parity_engine}", flush=True)
        code = run_parity_report(
            best_checkpoint,
            exported,
            args.parity_engine,
            args.holdout_dataset,
            args.parity_limit,
            args.engine_tolerance,
            run_dir / "parity_report.txt",
        )
        if code != 0:
            return code

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
