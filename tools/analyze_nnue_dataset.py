#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import math
from collections import Counter
from pathlib import Path

import chess


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Print sanity statistics for an NNUE labeled-position dataset.")
    parser.add_argument("--dataset", required=True, type=Path)
    parser.add_argument("--top-sources", type=int, default=12)
    return parser.parse_args()


def bucket(score: int) -> str:
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


def source_group(source: str) -> str:
    if source.startswith("random:"):
        return "random"
    if ":game" in source and ":ply" in source:
        return "pgn"
    return Path(source.split(":", 1)[0]).name


def main() -> int:
    args = parse_args()
    rows = 0
    invalid = 0
    scores: list[int] = []
    side_to_move = Counter()
    score_buckets = Counter()
    phase = Counter()
    sources = Counter()
    pieces = Counter()

    with args.dataset.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            rows += 1
            try:
                board = chess.Board(row["fen"])
                score = int(float(row["score_cp"]))
            except (KeyError, ValueError):
                invalid += 1
                continue
            scores.append(score)
            side_to_move["white" if board.turn == chess.WHITE else "black"] += 1
            score_buckets[bucket(score)] += 1
            phase[phase_bucket(board)] += 1
            sources[source_group(row.get("source", ""))] += 1
            pieces[sum(1 for _ in board.piece_map())] += 1

    if not scores:
        print(f"dataset={args.dataset} rows={rows} invalid={invalid} usable=0")
        return 1

    mean = sum(scores) / len(scores)
    variance = sum((score - mean) * (score - mean) for score in scores) / len(scores)
    sorted_scores = sorted(scores)
    p05 = sorted_scores[math.floor(0.05 * (len(sorted_scores) - 1))]
    p50 = sorted_scores[math.floor(0.50 * (len(sorted_scores) - 1))]
    p95 = sorted_scores[math.floor(0.95 * (len(sorted_scores) - 1))]

    print(f"dataset {args.dataset}")
    print(f"rows {rows} usable {len(scores)} invalid {invalid}")
    print(f"score_cp mean {mean:.1f} stdev {math.sqrt(variance):.1f} p05 {p05} p50 {p50} p95 {p95}")
    print("side_to_move " + " ".join(f"{key}={value}" for key, value in side_to_move.most_common()))
    print("score_buckets " + " ".join(f"{key}={score_buckets[key]}" for key in ("equal", "small", "clear", "large", "decisive")))
    print("phase " + " ".join(f"{key}={value}" for key, value in phase.most_common()))
    print("top_sources " + " ".join(f"{key}={value}" for key, value in sources.most_common(args.top_sources)))
    print("piece_counts " + " ".join(f"{key}={value}" for key, value in sorted(pieces.items())))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
