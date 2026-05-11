#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import os
import sys
from concurrent.futures import FIRST_COMPLETED, ProcessPoolExecutor, wait
from pathlib import Path

import chess
import torch

from nnue_model import (
    FEATURE_SET_HALFKA_V2_HM_FULL_THREATS,
    FULL_THREATS_MAX_ACTIVE,
    SUPPORTED_FEATURE_SETS,
    active_feature_blocks,
    active_features,
    feature_count_for,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build a compact tensor feature cache for large NNUE CSV datasets.")
    parser.add_argument("--dataset", required=True, type=Path, help="Canonical CSV with fen and score_cp columns.")
    parser.add_argument("--output", required=True, type=Path, help="Output .pt feature cache.")
    parser.add_argument("--clip-cp", type=int, default=1500)
    parser.add_argument("--max-features", type=int, default=30, help="Maximum active features per perspective; 0 selects a feature-set default.")
    parser.add_argument(
        "--feature-set",
        choices=SUPPORTED_FEATURE_SETS,
        default="halfkp-v1",
        help="NNUE feature mapping to encode.",
    )
    parser.add_argument("--progress-every", type=int, default=100000)
    parser.add_argument("--workers", type=int, default=1, help="Number of worker processes used to encode rows.")
    parser.add_argument("--chunk-size", type=int, default=10000, help="Rows per worker task when --workers > 1.")
    parser.add_argument(
        "--shard-rows",
        type=int,
        default=0,
        help="Write a JSON manifest plus shard .pt files with at most this many cached rows per shard.",
    )
    return parser.parse_args()


def count_rows(path: Path) -> int:
    with path.open(encoding="utf-8", newline="") as handle:
        return max(0, sum(1 for _ in handle) - 1)


def encode_rows(
    rows: list[dict[str, str]],
    clip_cp: int,
    feature_set: str,
    max_placement_features: int,
    max_threat_features: int,
    split_blocks: bool,
) -> dict[str, object]:
    encoded: list[tuple[list[int], list[int], list[int], list[int], int, float]] = []
    invalid = 0
    too_wide = 0
    for row in rows:
        try:
            board = chess.Board(row["fen"])
            score = max(-clip_cp, min(clip_cp, float(row["score_cp"])))
            if split_blocks:
                white, white_threat = active_feature_blocks(board, chess.WHITE, feature_set)
                black, black_threat = active_feature_blocks(board, chess.BLACK, feature_set)
            else:
                white = active_features(board, chess.WHITE, feature_set)
                black = active_features(board, chess.BLACK, feature_set)
                white_threat = []
                black_threat = []
        except (KeyError, ValueError):
            invalid += 1
            continue
        if (
            len(white) > max_placement_features
            or len(black) > max_placement_features
            or len(white_threat) > max_threat_features
            or len(black_threat) > max_threat_features
        ):
            too_wide += 1
            continue
        encoded.append((white, black, white_threat, black_threat, 1 if board.turn == chess.WHITE else -1, score))

    result: dict[str, object] = {
        "input_rows": len(rows),
        "written": len(encoded),
        "invalid": invalid,
        "too_wide": too_wide,
        "encoded": encoded,
    }
    return result


def chunked_reader(reader: csv.DictReader, chunk_size: int):
    chunk: list[dict[str, str]] = []
    for row in reader:
        chunk.append(row)
        if len(chunk) >= chunk_size:
            yield chunk
            chunk = []
    if chunk:
        yield chunk


class TensorSink:
    def __init__(
        self,
        rows: int,
        max_features: int,
        feature_set: str,
        feature_count: int,
        dataset: Path,
        max_placement_features: int,
        max_threat_features: int,
        clip_cp: int,
        split_blocks: bool,
    ) -> None:
        self.capacity = rows
        self.max_features = max_features
        self.feature_set = feature_set
        self.feature_count = feature_count
        self.dataset = dataset
        self.max_placement_features = max_placement_features
        self.max_threat_features = max_threat_features
        self.clip_cp = clip_cp
        self.split_blocks = split_blocks
        self.white_indices = torch.zeros((rows, max_placement_features), dtype=torch.int32)
        self.black_indices = torch.zeros((rows, max_placement_features), dtype=torch.int32)
        self.white_lengths = torch.zeros(rows, dtype=torch.int16)
        self.black_lengths = torch.zeros(rows, dtype=torch.int16)
        self.white_threat_indices = torch.zeros((rows, max_threat_features), dtype=torch.int32) if split_blocks else None
        self.black_threat_indices = torch.zeros((rows, max_threat_features), dtype=torch.int32) if split_blocks else None
        self.white_threat_lengths = torch.zeros(rows, dtype=torch.int16) if split_blocks else None
        self.black_threat_lengths = torch.zeros(rows, dtype=torch.int16) if split_blocks else None
        self.side_to_move = torch.empty(rows, dtype=torch.int8)
        self.target = torch.empty(rows, dtype=torch.float32)
        self.written = 0

    def full(self) -> bool:
        return self.written >= self.capacity

    def append(self, encoded: tuple[list[int], list[int], list[int], list[int], int, float]) -> None:
        white, black, white_threat, black_threat, stm, score = encoded
        if self.full():
            raise RuntimeError("cannot append to full TensorSink")
        row = self.written
        if white:
            self.white_indices[row, : len(white)] = torch.tensor(white, dtype=torch.int32)
        if black:
            self.black_indices[row, : len(black)] = torch.tensor(black, dtype=torch.int32)
        self.white_lengths[row] = len(white)
        self.black_lengths[row] = len(black)
        self.side_to_move[row] = stm
        self.target[row] = score
        if self.split_blocks:
            assert self.white_threat_indices is not None
            assert self.black_threat_indices is not None
            assert self.white_threat_lengths is not None
            assert self.black_threat_lengths is not None
            if white_threat:
                self.white_threat_indices[row, : len(white_threat)] = torch.tensor(white_threat, dtype=torch.int32)
            if black_threat:
                self.black_threat_indices[row, : len(black_threat)] = torch.tensor(black_threat, dtype=torch.int32)
            self.white_threat_lengths[row] = len(white_threat)
            self.black_threat_lengths[row] = len(black_threat)
        self.written += 1

    def cache(self) -> dict[str, object]:
        cache = {
            "format_version": 4 if self.split_blocks else 3,
            "feature_set": self.feature_set,
            "feature_count": self.feature_count,
            "dataset": str(self.dataset),
            "rows": self.written,
            "max_features": self.max_features,
            "max_placement_features": self.max_placement_features,
            "max_threat_features": self.max_threat_features,
            "clip_cp": self.clip_cp,
            "white_indices": self.white_indices[: self.written].contiguous(),
            "white_lengths": self.white_lengths[: self.written].contiguous(),
            "black_indices": self.black_indices[: self.written].contiguous(),
            "black_lengths": self.black_lengths[: self.written].contiguous(),
            "side_to_move": self.side_to_move[: self.written].contiguous(),
            "target": self.target[: self.written].contiguous(),
        }
        if self.split_blocks:
            assert self.white_threat_indices is not None
            assert self.black_threat_indices is not None
            assert self.white_threat_lengths is not None
            assert self.black_threat_lengths is not None
            cache.update(
                {
                    "white_threat_indices": self.white_threat_indices[: self.written].contiguous(),
                    "white_threat_lengths": self.white_threat_lengths[: self.written].contiguous(),
                    "black_threat_indices": self.black_threat_indices[: self.written].contiguous(),
                    "black_threat_lengths": self.black_threat_lengths[: self.written].contiguous(),
                }
            )
        return cache


def encoded_results(args: argparse.Namespace, reader: csv.DictReader, max_placement_features: int, max_threat_features: int, split_blocks: bool):
    if args.workers == 1:
        for chunk in chunked_reader(reader, args.chunk_size):
            yield encode_rows(chunk, args.clip_cp, args.feature_set, max_placement_features, max_threat_features, split_blocks)
        return

    workers = min(args.workers, os.cpu_count() or args.workers)
    with ProcessPoolExecutor(max_workers=workers) as executor:
        chunks = iter(chunked_reader(reader, args.chunk_size))
        pending = set()
        max_pending = workers * 2
        while True:
            while len(pending) < max_pending:
                try:
                    chunk = next(chunks)
                except StopIteration:
                    break
                pending.add(
                    executor.submit(
                        encode_rows,
                        chunk,
                        args.clip_cp,
                        args.feature_set,
                        max_placement_features,
                        max_threat_features,
                        split_blocks,
                    )
                )
            if not pending:
                break
            done, pending = wait(pending, return_when=FIRST_COMPLETED)
            for future in done:
                yield future.result()


def build_sharded_cache(
    args: argparse.Namespace,
    total_input: int,
    max_placement_features: int,
    max_threat_features: int,
    split_blocks: bool,
) -> int:
    shard_dir = args.output.with_suffix("")
    shard_dir = shard_dir.parent / f"{shard_dir.name}_shards"
    shard_dir.mkdir(parents=True, exist_ok=True)
    feature_count = feature_count_for(args.feature_set)
    shards: list[dict[str, object]] = []
    sink = TensorSink(
        args.shard_rows,
        args.max_features,
        args.feature_set,
        feature_count,
        args.dataset,
        max_placement_features,
        max_threat_features,
        args.clip_cp,
        split_blocks,
    )
    processed = 0
    written = 0
    invalid = 0
    too_wide = 0

    def save_shard() -> None:
        nonlocal sink
        if sink.written == 0:
            return
        shard_path = shard_dir / f"shard_{len(shards):05d}.pt"
        torch.save(sink.cache(), shard_path)
        shards.append({"path": str(shard_path), "rows": sink.written})
        print(f"[cache] saved shard={shard_path} rows={sink.written}", file=sys.stderr, flush=True)
        sink = TensorSink(
            args.shard_rows,
            args.max_features,
            args.feature_set,
            feature_count,
            args.dataset,
            max_placement_features,
            max_threat_features,
            args.clip_cp,
            split_blocks,
        )

    with args.dataset.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for result in encoded_results(args, reader, max_placement_features, max_threat_features, split_blocks):
            encoded: list[tuple[list[int], list[int], list[int], list[int], int, float]] = result["encoded"]  # type: ignore[assignment]
            for item in encoded:
                if sink.full():
                    save_shard()
                sink.append(item)
                written += 1
            invalid += int(result["invalid"])
            too_wide += int(result["too_wide"])
            processed += int(result["input_rows"])
            if processed % args.progress_every < int(result["input_rows"]) or processed == total_input:
                print(
                    f"[cache] processed={processed}/{total_input} cached={written} "
                    f"invalid={invalid} too_wide={too_wide}",
                    file=sys.stderr,
                    flush=True,
                )
    save_shard()
    if written == 0:
        print("no usable positions cached", file=sys.stderr)
        return 1

    manifest = {
        "format": "chessengine.nnue_feature_cache_shards",
        "format_version": 1,
        "feature_set": args.feature_set,
        "feature_count": feature_count,
        "dataset": str(args.dataset),
        "rows": written,
        "max_features": args.max_features,
        "max_placement_features": max_placement_features,
        "max_threat_features": max_threat_features,
        "clip_cp": args.clip_cp,
        "shard_rows": args.shard_rows,
        "shards": shards,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote sharded feature cache manifest {args.output} rows={written} shards={len(shards)} invalid={invalid} too_wide={too_wide}")
    return 0


def main() -> int:
    args = parse_args()
    if args.clip_cp <= 0:
        print("--clip-cp must be positive", file=sys.stderr)
        return 2
    if args.max_features < 0:
        print("--max-features must be non-negative", file=sys.stderr)
        return 2
    if args.max_features == 0:
        args.max_features = 160 if args.feature_set == FEATURE_SET_HALFKA_V2_HM_FULL_THREATS else 30
    if args.progress_every <= 0:
        print("--progress-every must be positive", file=sys.stderr)
        return 2
    if args.workers <= 0:
        print("--workers must be positive", file=sys.stderr)
        return 2
    if args.chunk_size <= 0:
        print("--chunk-size must be positive", file=sys.stderr)
        return 2
    if args.shard_rows < 0:
        print("--shard-rows must not be negative", file=sys.stderr)
        return 2

    total_input = count_rows(args.dataset)
    split_blocks = args.feature_set == FEATURE_SET_HALFKA_V2_HM_FULL_THREATS
    max_placement_features = 32 if split_blocks else args.max_features
    max_threat_features = FULL_THREATS_MAX_ACTIVE if split_blocks else 0
    print(
        f"[cache] allocating rows={total_input} max_features={args.max_features} "
        f"feature_set={args.feature_set} feature_count={feature_count_for(args.feature_set)} dataset={args.dataset}",
        file=sys.stderr,
        flush=True,
    )
    if args.shard_rows > 0:
        return build_sharded_cache(args, total_input, max_placement_features, max_threat_features, split_blocks)

    white_indices = torch.zeros((total_input, max_placement_features), dtype=torch.int32)
    black_indices = torch.zeros((total_input, max_placement_features), dtype=torch.int32)
    white_lengths = torch.zeros(total_input, dtype=torch.int16)
    black_lengths = torch.zeros(total_input, dtype=torch.int16)
    white_threat_indices = torch.zeros((total_input, max_threat_features), dtype=torch.int32) if split_blocks else None
    black_threat_indices = torch.zeros((total_input, max_threat_features), dtype=torch.int32) if split_blocks else None
    white_threat_lengths = torch.zeros(total_input, dtype=torch.int16) if split_blocks else None
    black_threat_lengths = torch.zeros(total_input, dtype=torch.int16) if split_blocks else None
    side_to_move = torch.empty(total_input, dtype=torch.int8)
    target = torch.empty(total_input, dtype=torch.float32)

    written = 0
    invalid = 0
    too_wide = 0
    processed = 0

    def append_result(result: dict[str, object]) -> None:
        nonlocal written, invalid, too_wide, processed
        encoded: list[tuple[list[int], list[int], list[int], list[int], int, float]] = result["encoded"]  # type: ignore[assignment]
        chunk_written = len(encoded)
        input_rows = int(result["input_rows"])
        for white, black, white_threat, black_threat, stm, score in encoded:
            if white:
                white_indices[written, : len(white)] = torch.tensor(white, dtype=torch.int32)
            if black:
                black_indices[written, : len(black)] = torch.tensor(black, dtype=torch.int32)
            white_lengths[written] = len(white)
            black_lengths[written] = len(black)
            side_to_move[written] = stm
            target[written] = score
            if split_blocks:
                assert white_threat_indices is not None
                assert black_threat_indices is not None
                assert white_threat_lengths is not None
                assert black_threat_lengths is not None
                if white_threat:
                    white_threat_indices[written, : len(white_threat)] = torch.tensor(white_threat, dtype=torch.int32)
                if black_threat:
                    black_threat_indices[written, : len(black_threat)] = torch.tensor(black_threat, dtype=torch.int32)
                white_threat_lengths[written] = len(white_threat)
                black_threat_lengths[written] = len(black_threat)
            written += 1
        invalid += int(result["invalid"])
        too_wide += int(result["too_wide"])
        processed += input_rows
        if processed % args.progress_every < input_rows or processed == total_input:
            print(
                f"[cache] processed={processed}/{total_input} cached={written} "
                f"invalid={invalid} too_wide={too_wide}",
                file=sys.stderr,
                flush=True,
            )

    with args.dataset.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        if args.workers == 1:
            for chunk in chunked_reader(reader, args.chunk_size):
                append_result(
                    encode_rows(
                        chunk,
                        args.clip_cp,
                        args.feature_set,
                        max_placement_features,
                        max_threat_features,
                        split_blocks,
                    )
                )
        else:
            workers = min(args.workers, os.cpu_count() or args.workers)
            with ProcessPoolExecutor(max_workers=workers) as executor:
                chunks = iter(chunked_reader(reader, args.chunk_size))
                pending = set()
                max_pending = workers * 2
                while True:
                    while len(pending) < max_pending:
                        try:
                            chunk = next(chunks)
                        except StopIteration:
                            break
                        pending.add(
                            executor.submit(
                                encode_rows,
                                chunk,
                                args.clip_cp,
                                args.feature_set,
                                max_placement_features,
                                max_threat_features,
                                split_blocks,
                            )
                        )
                    if not pending:
                        break
                    done, pending = wait(pending, return_when=FIRST_COMPLETED)
                    for future in done:
                        append_result(future.result())

    if written == 0:
        print("no usable positions cached", file=sys.stderr)
        return 1

    cache = {
        "format_version": 4 if split_blocks else 3,
        "feature_set": args.feature_set,
        "feature_count": feature_count_for(args.feature_set),
        "dataset": str(args.dataset),
        "rows": written,
        "max_features": args.max_features,
        "max_placement_features": max_placement_features,
        "max_threat_features": max_threat_features,
        "clip_cp": args.clip_cp,
        "white_indices": white_indices[:written].contiguous(),
        "white_lengths": white_lengths[:written].contiguous(),
        "black_indices": black_indices[:written].contiguous(),
        "black_lengths": black_lengths[:written].contiguous(),
        "side_to_move": side_to_move[:written].contiguous(),
        "target": target[:written].contiguous(),
    }
    if split_blocks:
        assert white_threat_indices is not None
        assert black_threat_indices is not None
        assert white_threat_lengths is not None
        assert black_threat_lengths is not None
        cache.update(
            {
                "white_threat_indices": white_threat_indices[:written].contiguous(),
                "white_threat_lengths": white_threat_lengths[:written].contiguous(),
                "black_threat_indices": black_threat_indices[:written].contiguous(),
                "black_threat_lengths": black_threat_lengths[:written].contiguous(),
            }
        )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    print(f"[cache] saving output={args.output} rows={written}", file=sys.stderr, flush=True)
    torch.save(cache, args.output)
    print(f"wrote feature cache {args.output} rows={written} invalid={invalid} too_wide={too_wide}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
