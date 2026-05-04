#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

import chess
import torch

from nnue_model import evaluate_checkpoint_white_perspective, load_binary, load_checkpoint


EVAL_RE = re.compile(r"\bnnue_total (-?\d+)\b")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check Python checkpoint, exported NNUE, and C++ engine NNUE inference parity.")
    parser.add_argument("--checkpoint", required=True, type=Path)
    parser.add_argument("--nnue", required=True, type=Path)
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--dataset", type=Path, help="CSV with fen column.")
    parser.add_argument("--epd", type=Path, help="EPD file to sample.")
    parser.add_argument("--limit", type=int, default=128)
    parser.add_argument("--engine-tolerance", type=int, default=0)
    return parser.parse_args()


def dataset_fens(path: Path, limit: int) -> list[str]:
    fens: list[str] = []
    with path.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            fen = row.get("fen")
            if fen:
                fens.append(fen)
                if len(fens) >= limit:
                    break
    return fens


def epd_fens(path: Path, limit: int) -> list[str]:
    fens: list[str] = []
    with path.open(encoding="utf-8") as handle:
        for raw in handle:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            fields = line.split()
            if len(fields) >= 4:
                fens.append(" ".join(fields[:4]) + " 0 1")
                if len(fens) >= limit:
                    break
    return fens


def engine_nnue_scores(engine_path: Path, nnue_path: Path, fens: list[str]) -> list[int]:
    commands = [
        "uci",
        f"setoption name NnueFile value {nnue_path}",
        "setoption name EvalType value nnue",
        "isready",
    ]
    for fen in fens:
        commands.append(f"position fen {fen}")
        commands.append("eval")
    commands.append("quit")

    completed = subprocess.run(
        [str(engine_path)],
        input="\n".join(commands) + "\n",
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=max(20, len(fens) // 10 + 20),
    )
    if completed.returncode != 0:
        print(completed.stderr, file=sys.stderr)
        raise RuntimeError(f"engine exited with {completed.returncode}")

    scores: list[int] = []
    for line in completed.stdout.splitlines():
        if "info string eval" not in line:
            continue
        match = EVAL_RE.search(line)
        if match is None:
            raise RuntimeError(f"missing nnue_total in eval output: {line}")
        scores.append(int(match.group(1)))
    if len(scores) != len(fens):
        raise RuntimeError(f"expected {len(fens)} engine scores, got {len(scores)}")
    return scores


def main() -> int:
    args = parse_args()
    if args.limit <= 0:
        print("--limit must be positive", file=sys.stderr)
        return 2
    if args.dataset is None and args.epd is None:
        print("provide --dataset or --epd", file=sys.stderr)
        return 2

    fens = dataset_fens(args.dataset, args.limit) if args.dataset is not None else epd_fens(args.epd, args.limit)
    if not fens:
        print("no FENs loaded", file=sys.stderr)
        return 1

    print(f"[parity] loading checkpoint={args.checkpoint}", file=sys.stderr, flush=True)
    model, metadata = load_checkpoint(args.checkpoint, torch.device("cpu"))
    print(f"[parity] checkpoint hidden={model.hidden_size} metadata={metadata}", file=sys.stderr, flush=True)
    print(f"[parity] loading binary={args.nnue}", file=sys.stderr, flush=True)
    binary = load_binary(args.nnue)
    print(f"[parity] binary hidden={binary.hidden_size}", file=sys.stderr, flush=True)
    print(f"[parity] querying engine={args.engine} positions={len(fens)}", file=sys.stderr, flush=True)
    engine_scores = engine_nnue_scores(args.engine, args.nnue, fens)

    engine_diffs: list[int] = []
    checkpoint_diffs: list[float] = []
    worst_engine = ("", 0, 0, 0)
    worst_checkpoint = ("", 0.0, 0.0, 0)
    for index, fen in enumerate(fens, start=1):
        board = chess.Board(fen)
        binary_score = binary.evaluate_white_perspective(board)
        checkpoint_score = evaluate_checkpoint_white_perspective(model, board, torch.device("cpu"))
        engine_score = engine_scores[index - 1]
        engine_diff = abs(engine_score - binary_score)
        checkpoint_diff = abs(checkpoint_score - binary_score)
        engine_diffs.append(engine_diff)
        checkpoint_diffs.append(checkpoint_diff)
        if engine_diff > worst_engine[1]:
            worst_engine = (fen, engine_diff, engine_score, binary_score)
        if checkpoint_diff > worst_checkpoint[1]:
            worst_checkpoint = (fen, checkpoint_diff, checkpoint_score, binary_score)

    max_engine = max(engine_diffs)
    avg_engine = sum(engine_diffs) / len(engine_diffs)
    max_checkpoint = max(checkpoint_diffs)
    avg_checkpoint = sum(checkpoint_diffs) / len(checkpoint_diffs)
    print(f"positions {len(fens)}")
    print(f"engine_binary_abs_diff max {max_engine} avg {avg_engine:.3f}")
    print(f"checkpoint_binary_abs_diff max {max_checkpoint:.3f} avg {avg_checkpoint:.3f}")
    if max_engine > args.engine_tolerance:
        print(
            f"engine parity FAILED worst_diff={worst_engine[1]} engine={worst_engine[2]} "
            f"binary={worst_engine[3]} fen={worst_engine[0]}",
            file=sys.stderr,
        )
        return 1
    print("engine parity ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
