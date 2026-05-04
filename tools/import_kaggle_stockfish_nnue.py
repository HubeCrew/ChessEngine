#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import hashlib
import sys
from collections import Counter
from pathlib import Path

import chess


SPLIT_SCALE = 1_000_000


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Import the Kaggle Stockfish NNUE CSV into canonical train/validation/holdout CSV files."
    )
    parser.add_argument("--train-input", required=True, type=Path, help="Kaggle train.csv with FEN and Evaluation columns.")
    parser.add_argument("--test-input", type=Path, help="Optional Kaggle test.csv with FEN column and empty labels.")
    parser.add_argument("--output-dir", required=True, type=Path, help="Directory for canonical CSV outputs.")
    parser.add_argument("--clip-cp", type=int, default=1500, help="Clip labels to this centipawn magnitude.")
    parser.add_argument("--validation-fraction", type=float, default=0.05)
    parser.add_argument("--holdout-fraction", type=float, default=0.05)
    parser.add_argument("--progress-every", type=int, default=100000)
    return parser.parse_args()


def detect_delimiter(path: Path) -> str:
    sample = path.open("rb").read(8192).decode("utf-8", errors="replace")
    try:
        return csv.Sniffer().sniff(sample, delimiters=",\t;").delimiter
    except csv.Error:
        return "\t" if "\t" in sample.splitlines()[0] else ","


def fen_key(board: chess.Board) -> str:
    return " ".join(board.fen(en_passant="fen").split()[:4])


def split_name(key: str, validation_fraction: float, holdout_fraction: float) -> str:
    digest = hashlib.sha256(key.encode("utf-8")).digest()
    value = int.from_bytes(digest[:8], "little") % SPLIT_SCALE
    holdout_cutoff = int(holdout_fraction * SPLIT_SCALE)
    validation_cutoff = holdout_cutoff + int(validation_fraction * SPLIT_SCALE)
    if value < holdout_cutoff:
        return "holdout"
    if value < validation_cutoff:
        return "validation"
    return "train"


def writer(path: Path, fieldnames: list[str]) -> tuple[csv.DictWriter, object]:
    path.parent.mkdir(parents=True, exist_ok=True)
    handle = path.open("w", encoding="utf-8", newline="")
    csv_writer = csv.DictWriter(handle, fieldnames=fieldnames)
    csv_writer.writeheader()
    return csv_writer, handle


def import_labeled(args: argparse.Namespace) -> Counter:
    delimiter = detect_delimiter(args.train_input)
    writers: dict[str, csv.DictWriter] = {}
    handles: list[object] = []
    for name in ("train", "validation", "holdout"):
        out_writer, handle = writer(args.output_dir / f"{name}.csv", ["fen", "score_cp", "source"])
        writers[name] = out_writer
        handles.append(handle)

    stats: Counter = Counter()
    seen: set[str] = set()
    try:
        with args.train_input.open(encoding="utf-8", newline="") as input_handle:
            reader = csv.DictReader(input_handle, delimiter=delimiter)
            for row_index, row in enumerate(reader, start=1):
                stats["input_labeled"] += 1
                try:
                    board = chess.Board(row["FEN"])
                    raw_score = int(float(row["Evaluation"]))
                except (KeyError, ValueError):
                    stats["invalid_labeled"] += 1
                    continue
                key = fen_key(board)
                if key in seen:
                    stats["duplicate_labeled"] += 1
                    continue
                seen.add(key)
                score = max(-args.clip_cp, min(args.clip_cp, raw_score))
                if score != raw_score:
                    stats["clipped_labeled"] += 1
                split = split_name(key, args.validation_fraction, args.holdout_fraction)
                writers[split].writerow(
                    {
                        "fen": board.fen(en_passant="fen"),
                        "score_cp": score,
                        "source": f"kaggle_stockfish_nnue:train:{row_index}",
                    }
                )
                stats[f"written_{split}"] += 1
                if args.progress_every > 0 and row_index % args.progress_every == 0:
                    print(
                        f"[kaggle-import] labeled rows={row_index} train={stats['written_train']} "
                        f"validation={stats['written_validation']} holdout={stats['written_holdout']} "
                        f"invalid={stats['invalid_labeled']} duplicate={stats['duplicate_labeled']}",
                        file=sys.stderr,
                        flush=True,
                    )
    finally:
        for handle in handles:
            handle.close()
    return stats


def import_unlabeled(args: argparse.Namespace) -> Counter:
    stats: Counter = Counter()
    if args.test_input is None:
        return stats
    delimiter = detect_delimiter(args.test_input)
    out_writer, handle = writer(args.output_dir / "unlabeled_test.csv", ["fen", "source"])
    seen: set[str] = set()
    try:
        with args.test_input.open(encoding="utf-8", newline="") as input_handle:
            reader = csv.DictReader(input_handle, delimiter=delimiter)
            for row_index, row in enumerate(reader, start=1):
                stats["input_unlabeled"] += 1
                try:
                    board = chess.Board(row["FEN"])
                except (KeyError, ValueError):
                    stats["invalid_unlabeled"] += 1
                    continue
                key = fen_key(board)
                if key in seen:
                    stats["duplicate_unlabeled"] += 1
                    continue
                seen.add(key)
                out_writer.writerow(
                    {
                        "fen": board.fen(en_passant="fen"),
                        "source": f"kaggle_stockfish_nnue:test:{row_index}",
                    }
                )
                stats["written_unlabeled"] += 1
    finally:
        handle.close()
    return stats


def write_report(path: Path, args: argparse.Namespace, stats: Counter) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write(f"train_input {args.train_input}\n")
        if args.test_input is not None:
            handle.write(f"test_input {args.test_input}\n")
        handle.write(f"output_dir {args.output_dir}\n")
        handle.write(f"clip_cp {args.clip_cp}\n")
        handle.write(f"validation_fraction {args.validation_fraction:.4f}\n")
        handle.write(f"holdout_fraction {args.holdout_fraction:.4f}\n")
        for key in sorted(stats):
            handle.write(f"{key} {stats[key]}\n")


def main() -> int:
    args = parse_args()
    if args.clip_cp <= 0:
        print("--clip-cp must be positive", file=sys.stderr)
        return 2
    if args.progress_every <= 0:
        print("--progress-every must be positive", file=sys.stderr)
        return 2
    if args.validation_fraction < 0.0 or args.holdout_fraction < 0.0:
        print("split fractions must not be negative", file=sys.stderr)
        return 2
    if args.validation_fraction + args.holdout_fraction >= 1.0:
        print("validation and holdout fractions must sum to less than 1", file=sys.stderr)
        return 2

    print(f"[kaggle-import] importing labeled train={args.train_input}", file=sys.stderr, flush=True)
    stats = import_labeled(args)
    if args.test_input is not None:
        print(f"[kaggle-import] importing unlabeled test={args.test_input}", file=sys.stderr, flush=True)
        stats.update(import_unlabeled(args))
    report = args.output_dir / "import_report.txt"
    write_report(report, args, stats)
    print(
        f"[kaggle-import] wrote train={stats['written_train']} validation={stats['written_validation']} "
        f"holdout={stats['written_holdout']} unlabeled={stats['written_unlabeled']}",
        file=sys.stderr,
        flush=True,
    )
    print(f"wrote canonical Kaggle dataset to {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
