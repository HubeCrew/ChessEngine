#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import math
import os
from collections import Counter
from concurrent.futures import FIRST_COMPLETED, ProcessPoolExecutor, wait
from pathlib import Path

import chess


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Print sanity statistics for an NNUE labeled-position dataset.")
    parser.add_argument("--dataset", required=True, type=Path)
    parser.add_argument("--top-sources", type=int, default=12)
    parser.add_argument("--workers", type=int, default=1, help="Number of worker processes used to parse rows.")
    parser.add_argument("--chunk-size", type=int, default=20000, help="Rows per worker task when --workers > 1.")
    parser.add_argument("--progress-every", type=int, default=500000, help="Progress interval in input rows. 0 disables progress.")
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
    if source.startswith("lichess_eval:"):
        return "lichess_eval"
    if source.startswith("kaggle_stockfish_nnue:"):
        return "kaggle_stockfish_nnue"
    if ":game" in source and ":ply" in source:
        return "pgn"
    return Path(source.split(":", 1)[0]).name


def empty_stats() -> dict[str, object]:
    return {
        "rows": 0,
        "invalid": 0,
        "scores": [],
        "side_to_move": Counter(),
        "score_buckets": Counter(),
        "phase": Counter(),
        "sources": Counter(),
        "pieces": Counter(),
    }


def analyze_rows(rows: list[dict[str, str]]) -> dict[str, object]:
    stats = empty_stats()
    scores: list[int] = stats["scores"]  # type: ignore[assignment]
    side_to_move: Counter = stats["side_to_move"]  # type: ignore[assignment]
    score_buckets: Counter = stats["score_buckets"]  # type: ignore[assignment]
    phase: Counter = stats["phase"]  # type: ignore[assignment]
    sources: Counter = stats["sources"]  # type: ignore[assignment]
    pieces: Counter = stats["pieces"]  # type: ignore[assignment]

    for row in rows:
        stats["rows"] = int(stats["rows"]) + 1
        try:
            board = chess.Board(row["fen"])
            score = int(float(row["score_cp"]))
        except (KeyError, ValueError):
            stats["invalid"] = int(stats["invalid"]) + 1
            continue
        scores.append(score)
        side_to_move["white" if board.turn == chess.WHITE else "black"] += 1
        score_buckets[bucket(score)] += 1
        phase[phase_bucket(board)] += 1
        sources[source_group(row.get("source", ""))] += 1
        pieces[sum(1 for _ in board.piece_map())] += 1
    return stats


def merge_stats(total: dict[str, object], update: dict[str, object]) -> None:
    total["rows"] = int(total["rows"]) + int(update["rows"])
    total["invalid"] = int(total["invalid"]) + int(update["invalid"])
    total_scores: list[int] = total["scores"]  # type: ignore[assignment]
    update_scores: list[int] = update["scores"]  # type: ignore[assignment]
    total_scores.extend(update_scores)
    for key in ("side_to_move", "score_buckets", "phase", "sources", "pieces"):
        total_counter: Counter = total[key]  # type: ignore[assignment]
        update_counter: Counter = update[key]  # type: ignore[assignment]
        total_counter.update(update_counter)


def chunked_reader(reader: csv.DictReader, chunk_size: int):
    chunk: list[dict[str, str]] = []
    for row in reader:
        chunk.append(row)
        if len(chunk) >= chunk_size:
            yield chunk
            chunk = []
    if chunk:
        yield chunk


def print_progress(processed: int, total_stats: dict[str, object], progress_every: int, force: bool = False) -> None:
    if progress_every <= 0:
        return
    if not force and processed % progress_every != 0:
        return
    usable = len(total_stats["scores"])  # type: ignore[arg-type]
    print(
        f"[analyze] processed={processed} usable={usable} invalid={total_stats['invalid']}",
        flush=True,
    )


def main() -> int:
    args = parse_args()
    if args.workers <= 0:
        print("--workers must be positive", flush=True)
        return 2
    if args.chunk_size <= 0:
        print("--chunk-size must be positive", flush=True)
        return 2

    total_stats = empty_stats()
    processed = 0

    with args.dataset.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        if args.workers == 1:
            for chunk in chunked_reader(reader, args.chunk_size):
                update = analyze_rows(chunk)
                merge_stats(total_stats, update)
                processed += len(chunk)
                print_progress(processed, total_stats, args.progress_every)
        else:
            workers = min(args.workers, os.cpu_count() or args.workers)
            with ProcessPoolExecutor(max_workers=workers) as executor:
                chunks = iter(chunked_reader(reader, args.chunk_size))
                pending = set()
                max_pending = workers * 2
                while True:
                    while len(pending) < max_pending:
                        try:
                            pending.add(executor.submit(analyze_rows, next(chunks)))
                        except StopIteration:
                            break
                    if not pending:
                        break
                    done, pending = wait(pending, return_when=FIRST_COMPLETED)
                    for future in done:
                        update = future.result()
                        merge_stats(total_stats, update)
                        processed += int(update["rows"])
                        print_progress(processed, total_stats, args.progress_every)

    scores: list[int] = total_stats["scores"]  # type: ignore[assignment]
    rows = int(total_stats["rows"])
    invalid = int(total_stats["invalid"])
    side_to_move: Counter = total_stats["side_to_move"]  # type: ignore[assignment]
    score_buckets: Counter = total_stats["score_buckets"]  # type: ignore[assignment]
    phase: Counter = total_stats["phase"]  # type: ignore[assignment]
    sources: Counter = total_stats["sources"]  # type: ignore[assignment]
    pieces: Counter = total_stats["pieces"]  # type: ignore[assignment]

    if not scores:
        print(f"dataset={args.dataset} rows={rows} invalid={invalid} usable=0")
        return 1

    print_progress(processed, total_stats, args.progress_every, force=True)

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
