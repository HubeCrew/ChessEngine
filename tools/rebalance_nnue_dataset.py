#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import random
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

import chess


DEFAULT_BUCKET_FRACTIONS = {
    "equal": 0.12,
    "small": 0.18,
    "clear": 0.32,
    "large": 0.26,
    "decisive": 0.12,
}
BUCKET_ORDER = ("equal", "small", "clear", "large", "decisive")


@dataclass(frozen=True)
class Row:
    fen: str
    score_cp: int
    source: str
    score_bucket: str
    source_group: str
    phase: str
    side_to_move: str
    input_index: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a deterministic, distribution-capped NNUE training dataset from raw labeled positions."
    )
    parser.add_argument("--input", required=True, type=Path, help="Raw CSV with fen, score_cp, and source columns.")
    parser.add_argument("--output", required=True, type=Path, help="Balanced output CSV.")
    parser.add_argument("--report", type=Path, help="Optional text report path. Defaults beside --output.")
    parser.add_argument("--limit", type=int, default=0, help="Maximum rows to write. 0 means all usable rows after caps.")
    parser.add_argument("--seed", type=int, default=1, help="Deterministic shuffle seed.")
    parser.add_argument(
        "--bucket-fractions",
        default=",".join(f"{key}={value}" for key, value in DEFAULT_BUCKET_FRACTIONS.items()),
        help="Target score bucket mix, for example equal=0.12,small=0.18,clear=0.32,large=0.26,decisive=0.12.",
    )
    parser.add_argument(
        "--max-source-fraction",
        type=float,
        default=0.70,
        help="Maximum fraction of the output allowed from one source group.",
    )
    parser.add_argument(
        "--max-phase-fraction",
        type=float,
        default=0.60,
        help="Maximum fraction of the output allowed from one phase bucket.",
    )
    parser.add_argument(
        "--max-side-fraction",
        type=float,
        default=0.55,
        help="Maximum fraction of the output allowed for one side to move.",
    )
    parser.add_argument("--progress-every", type=int, default=25000)
    return parser.parse_args()


def fen_key(board: chess.Board) -> str:
    return " ".join(board.fen(en_passant="fen").split()[:4])


def score_bucket(score: int) -> str:
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


def parse_bucket_fractions(raw: str) -> dict[str, float]:
    fractions: dict[str, float] = {}
    for item in raw.split(","):
        if not item:
            continue
        key, separator, value = item.partition("=")
        if not separator:
            raise ValueError(f"missing '=' in bucket fraction {item!r}")
        key = key.strip()
        if key not in BUCKET_ORDER:
            raise ValueError(f"unsupported score bucket {key!r}")
        amount = float(value)
        if amount < 0.0:
            raise ValueError(f"negative score bucket fraction for {key!r}")
        fractions[key] = amount
    for key in BUCKET_ORDER:
        fractions.setdefault(key, 0.0)
    total = sum(fractions.values())
    if total <= 0.0:
        raise ValueError("bucket fractions must sum to a positive value")
    return {key: value / total for key, value in fractions.items()}


def validate_fraction(name: str, value: float) -> None:
    if value <= 0.0 or value > 1.0:
        raise ValueError(f"{name} must be in the range (0, 1]")


def load_rows(path: Path, progress_every: int) -> tuple[list[Row], int, int]:
    rows: list[Row] = []
    seen: set[str] = set()
    invalid = 0
    duplicate = 0
    with path.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for input_index, raw in enumerate(reader, start=1):
            try:
                board = chess.Board(raw["fen"])
                score = int(float(raw["score_cp"]))
            except (KeyError, ValueError):
                invalid += 1
                continue
            key = fen_key(board)
            if key in seen:
                duplicate += 1
                continue
            seen.add(key)
            source = raw.get("source", "")
            rows.append(
                Row(
                    fen=board.fen(en_passant="fen"),
                    score_cp=score,
                    source=source,
                    score_bucket=score_bucket(score),
                    source_group=source_group(source),
                    phase=phase_bucket(board),
                    side_to_move="white" if board.turn == chess.WHITE else "black",
                    input_index=input_index,
                )
            )
            if progress_every > 0 and input_index % progress_every == 0:
                print(f"[rebalance] loaded input_rows={input_index} usable={len(rows)}", file=sys.stderr, flush=True)
    return rows, invalid, duplicate


def cap_count(limit: int, fraction: float) -> int:
    return max(1, int(limit * fraction))


def summarize(rows: list[Row]) -> dict[str, Counter]:
    return {
        "score_buckets": Counter(row.score_bucket for row in rows),
        "source_groups": Counter(row.source_group for row in rows),
        "phase": Counter(row.phase for row in rows),
        "side_to_move": Counter(row.side_to_move for row in rows),
    }


def counter_line(name: str, counter: Counter, keys: tuple[str, ...] | None = None) -> str:
    if keys is None:
        items = counter.most_common()
    else:
        items = [(key, counter[key]) for key in keys]
    return name + " " + " ".join(f"{key}={value}" for key, value in items)


def write_report(
    path: Path,
    input_path: Path,
    output_path: Path,
    rows: list[Row],
    selected: list[Row],
    invalid: int,
    duplicate: int,
    target_limit: int,
    bucket_fractions: dict[str, float],
    max_source_fraction: float,
    max_phase_fraction: float,
    max_side_fraction: float,
) -> None:
    before = summarize(rows)
    after = summarize(selected)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write(f"input {input_path}\n")
        handle.write(f"output {output_path}\n")
        handle.write(f"usable_input {len(rows)}\n")
        handle.write(f"invalid_input {invalid}\n")
        handle.write(f"duplicate_input {duplicate}\n")
        handle.write(f"target_limit {target_limit}\n")
        handle.write(f"selected {len(selected)}\n")
        handle.write(
            "bucket_fractions "
            + " ".join(f"{key}={bucket_fractions[key]:.4f}" for key in BUCKET_ORDER)
            + "\n"
        )
        handle.write(f"max_source_fraction {max_source_fraction:.4f}\n")
        handle.write(f"max_phase_fraction {max_phase_fraction:.4f}\n")
        handle.write(f"max_side_fraction {max_side_fraction:.4f}\n")
        handle.write("\n[input]\n")
        handle.write(counter_line("score_buckets", before["score_buckets"], BUCKET_ORDER) + "\n")
        handle.write(counter_line("source_groups", before["source_groups"]) + "\n")
        handle.write(counter_line("phase", before["phase"]) + "\n")
        handle.write(counter_line("side_to_move", before["side_to_move"]) + "\n")
        handle.write("\n[output]\n")
        handle.write(counter_line("score_buckets", after["score_buckets"], BUCKET_ORDER) + "\n")
        handle.write(counter_line("source_groups", after["source_groups"]) + "\n")
        handle.write(counter_line("phase", after["phase"]) + "\n")
        handle.write(counter_line("side_to_move", after["side_to_move"]) + "\n")


def choose_rows(
    rows: list[Row],
    limit: int,
    bucket_fractions: dict[str, float],
    max_source_fraction: float,
    max_phase_fraction: float,
    max_side_fraction: float,
    seed: int,
) -> list[Row]:
    rng = random.Random(seed)
    shuffled = list(rows)
    rng.shuffle(shuffled)

    source_cap = cap_count(limit, max_source_fraction)
    phase_cap = cap_count(limit, max_phase_fraction)
    side_cap = cap_count(limit, max_side_fraction)
    bucket_targets = {key: int(limit * bucket_fractions[key]) for key in BUCKET_ORDER}
    bucket_targets[BUCKET_ORDER[-1]] += limit - sum(bucket_targets.values())

    selected: list[Row] = []
    used_indices: set[int] = set()
    counts = summarize([])

    def can_take(row: Row, enforce_bucket_target: bool, enforce_source: bool, enforce_phase_side: bool) -> bool:
        if row.input_index in used_indices:
            return False
        if enforce_bucket_target and counts["score_buckets"][row.score_bucket] >= bucket_targets[row.score_bucket]:
            return False
        if enforce_source and counts["source_groups"][row.source_group] >= source_cap:
            return False
        if enforce_phase_side:
            if counts["phase"][row.phase] >= phase_cap:
                return False
            if counts["side_to_move"][row.side_to_move] >= side_cap:
                return False
        return True

    def take(row: Row) -> None:
        selected.append(row)
        used_indices.add(row.input_index)
        counts["score_buckets"][row.score_bucket] += 1
        counts["source_groups"][row.source_group] += 1
        counts["phase"][row.phase] += 1
        counts["side_to_move"][row.side_to_move] += 1

    by_bucket: dict[str, list[Row]] = {key: [] for key in BUCKET_ORDER}
    for row in shuffled:
        by_bucket[row.score_bucket].append(row)

    for bucket in BUCKET_ORDER:
        for row in by_bucket[bucket]:
            if len(selected) >= limit:
                return sorted(selected, key=lambda item: item.input_index)
            if can_take(row, enforce_bucket_target=True, enforce_source=True, enforce_phase_side=True):
                take(row)

    for enforce_phase_side in (True, False):
        for row in shuffled:
            if len(selected) >= limit:
                return sorted(selected, key=lambda item: item.input_index)
            if can_take(row, enforce_bucket_target=False, enforce_source=True, enforce_phase_side=enforce_phase_side):
                take(row)

    return sorted(selected, key=lambda item: item.input_index)


def write_csv(path: Path, rows: list[Row]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=["fen", "score_cp", "source"])
        writer.writeheader()
        for row in rows:
            writer.writerow({"fen": row.fen, "score_cp": row.score_cp, "source": row.source})


def main() -> int:
    args = parse_args()
    try:
        bucket_fractions = parse_bucket_fractions(args.bucket_fractions)
        validate_fraction("--max-source-fraction", args.max_source_fraction)
        validate_fraction("--max-phase-fraction", args.max_phase_fraction)
        validate_fraction("--max-side-fraction", args.max_side_fraction)
    except ValueError as error:
        print(error, file=sys.stderr)
        return 2
    if args.limit < 0:
        print("--limit must not be negative", file=sys.stderr)
        return 2

    print(f"[rebalance] loading input={args.input}", file=sys.stderr, flush=True)
    rows, invalid, duplicate = load_rows(args.input, args.progress_every)
    if not rows:
        print("no usable rows", file=sys.stderr)
        return 1

    target_limit = args.limit if args.limit > 0 else len(rows)
    target_limit = min(target_limit, len(rows))
    print(
        f"[rebalance] selecting target={target_limit} usable={len(rows)} invalid={invalid} duplicate={duplicate}",
        file=sys.stderr,
        flush=True,
    )
    selected = choose_rows(
        rows=rows,
        limit=target_limit,
        bucket_fractions=bucket_fractions,
        max_source_fraction=args.max_source_fraction,
        max_phase_fraction=args.max_phase_fraction,
        max_side_fraction=args.max_side_fraction,
        seed=args.seed,
    )
    write_csv(args.output, selected)
    report = args.report if args.report is not None else args.output.with_name(args.output.stem + "_report.txt")
    write_report(
        report,
        args.input,
        args.output,
        rows,
        selected,
        invalid,
        duplicate,
        target_limit,
        bucket_fractions,
        args.max_source_fraction,
        args.max_phase_fraction,
        args.max_side_fraction,
    )

    print(f"[rebalance] wrote output={args.output} rows={len(selected)}", file=sys.stderr, flush=True)
    print(f"[rebalance] wrote report={report}", file=sys.stderr, flush=True)
    if len(selected) < target_limit:
        print(
            f"[rebalance] warning selected={len(selected)} below target={target_limit}; "
            "caps are stricter than the available source mix",
            file=sys.stderr,
            flush=True,
        )
    print(f"wrote {len(selected)} balanced rows to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
