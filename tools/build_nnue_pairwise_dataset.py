#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import random
import re
import sys
from pathlib import Path

import chess


REF_DELTA_RE = re.compile(r"\bref_delta\s+(-?\d+)\b")
SEVERITY_RE = re.compile(r"\bseverity\s+(-?\d+)\b")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Build pairwise NNUE fine-tuning rows from EPD bm/am operations. "
            "Each output row says the best-move child must outrank a negative child."
        )
    )
    parser.add_argument("--epd", action="append", required=True, type=Path, help="Input EPD suite with bm and am operations.")
    parser.add_argument("--output", required=True, type=Path, help="Output pairwise train CSV.")
    parser.add_argument("--validation-output", type=Path, help="Optional validation pair CSV.")
    parser.add_argument("--validation-fraction", type=float, default=0.0)
    parser.add_argument(
        "--benchmark-results",
        action="append",
        type=Path,
        help="Optional chess_bench CSV. Mismatched bestmove rows are added as hard negatives.",
    )
    parser.add_argument("--exclude-epd", action="append", type=Path, help="Exclude ids present in these EPD suites.")
    parser.add_argument("--limit", type=int, default=0, help="Maximum written pairs after shuffling. 0 means no cap.")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--default-margin-cp", type=int, default=250)
    parser.add_argument("--min-margin-cp", type=int, default=80)
    parser.add_argument("--max-margin-cp", type=int, default=3000)
    parser.add_argument(
        "--extra-legal-negatives",
        type=int,
        default=0,
        help="Add this many deterministic legal non-best/non-am moves per position.",
    )
    return parser.parse_args()


def normalize_moves(value: object) -> list[chess.Move]:
    if value is None:
        return []
    if isinstance(value, chess.Move):
        return [value]
    if isinstance(value, list):
        return [item for item in value if isinstance(item, chess.Move)]
    return []


def epd_id(ops: dict[str, object], fallback: str) -> str:
    value = ops.get("id")
    return str(value) if value is not None else fallback


def epd_text_comments(ops: dict[str, object]) -> str:
    return " ".join(str(value) for key, value in ops.items() if key.startswith("c"))


def margin_from_ops(ops: dict[str, object], args: argparse.Namespace) -> int:
    comments = epd_text_comments(ops)
    candidates: list[int] = []
    for pattern in (REF_DELTA_RE, SEVERITY_RE):
        match = pattern.search(comments)
        if match is not None:
            candidates.append(abs(int(match.group(1))))
    margin = max(candidates) if candidates else args.default_margin_cp
    return max(args.min_margin_cp, min(args.max_margin_cp, margin))


def read_benchmark_hard_negatives(paths: list[Path] | None) -> dict[str, set[str]]:
    hard_negatives: dict[str, set[str]] = {}
    for path in paths or []:
        with path.open(encoding="utf-8", newline="") as handle:
            reader = csv.DictReader(handle)
            for row in reader:
                if row.get("matched", "").lower() == "true":
                    continue
                position_id = row.get("id", "")
                bestmove = row.get("bestmove", "")
                if position_id and bestmove:
                    hard_negatives.setdefault(position_id, set()).add(bestmove)
    return hard_negatives


def excluded_ids(paths: list[Path] | None) -> set[str]:
    excluded: set[str] = set()
    for path in paths or []:
        with path.open(encoding="utf-8") as handle:
            for index, line in enumerate(handle, start=1):
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                board = chess.Board()
                try:
                    ops = board.set_epd(line)
                except ValueError:
                    continue
                excluded.add(epd_id(ops, f"{path.name}:{index}"))
    return excluded


def child_fen(board: chess.Board, move: chess.Move) -> str:
    child = board.copy(stack=False)
    child.push(move)
    return child.fen()


def add_pair(
    rows: list[dict[str, str]],
    board: chess.Board,
    position_id: str,
    theme: str,
    source: str,
    positive: chess.Move,
    negative: chess.Move,
    margin_cp: int,
) -> None:
    if positive == negative:
        return
    if positive not in board.legal_moves or negative not in board.legal_moves:
        return
    parent_side = "white" if board.turn == chess.WHITE else "black"
    side_sign = 1 if board.turn == chess.WHITE else -1
    rows.append(
        {
            "id": position_id,
            "theme": theme,
            "source": source,
            "parent_fen": board.fen(),
            "parent_side": parent_side,
            "side_sign": str(side_sign),
            "positive_move": positive.uci(),
            "negative_move": negative.uci(),
            "positive_child_fen": child_fen(board, positive),
            "negative_child_fen": child_fen(board, negative),
            "margin_cp": str(margin_cp),
            "positive_target_cp": str(side_sign * margin_cp // 2),
            "negative_target_cp": str(-side_sign * margin_cp // 2),
        }
    )


def build_rows(args: argparse.Namespace) -> list[dict[str, str]]:
    hard_negatives = read_benchmark_hard_negatives(args.benchmark_results)
    skip_ids = excluded_ids(args.exclude_epd)
    rows: list[dict[str, str]] = []
    skipped = 0
    invalid = 0
    for path in args.epd:
        with path.open(encoding="utf-8") as handle:
            for index, line in enumerate(handle, start=1):
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                board = chess.Board()
                try:
                    ops = board.set_epd(line)
                except ValueError:
                    invalid += 1
                    continue
                position_id = epd_id(ops, f"{path.name}:{index}")
                if position_id in skip_ids:
                    skipped += 1
                    continue
                positives = normalize_moves(ops.get("bm"))
                if not positives:
                    invalid += 1
                    continue
                negatives = normalize_moves(ops.get("am"))
                for uci in sorted(hard_negatives.get(position_id, set())):
                    try:
                        move = chess.Move.from_uci(uci)
                    except ValueError:
                        continue
                    if move in board.legal_moves:
                        negatives.append(move)
                if args.extra_legal_negatives > 0:
                    blocked = {move.uci() for move in positives + negatives}
                    extras = [move for move in board.legal_moves if move.uci() not in blocked]
                    extras.sort(key=lambda move: move.uci())
                    negatives.extend(extras[: args.extra_legal_negatives])
                if not negatives:
                    invalid += 1
                    continue
                theme = str(ops.get("theme", ""))
                margin_cp = margin_from_ops(ops, args)
                for positive in positives:
                    for negative in negatives:
                        add_pair(rows, board, position_id, theme, str(path), positive, negative, margin_cp)
    print(f"[pairs] built={len(rows)} invalid={invalid} skipped={skipped}", file=sys.stderr, flush=True)
    return rows


def write_rows(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = [
        "id",
        "theme",
        "source",
        "parent_fen",
        "parent_side",
        "side_sign",
        "positive_move",
        "negative_move",
        "positive_child_fen",
        "negative_child_fen",
        "margin_cp",
        "positive_target_cp",
        "negative_target_cp",
    ]
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    print(f"wrote pairwise dataset {path} rows={len(rows)}")


def main() -> int:
    args = parse_args()
    if args.default_margin_cp <= 0 or args.min_margin_cp <= 0 or args.max_margin_cp <= 0:
        print("margins must be positive", file=sys.stderr)
        return 2
    if args.min_margin_cp > args.max_margin_cp:
        print("--min-margin-cp must not exceed --max-margin-cp", file=sys.stderr)
        return 2
    if not 0.0 <= args.validation_fraction < 1.0:
        print("--validation-fraction must be in [0, 1)", file=sys.stderr)
        return 2
    if args.validation_fraction > 0.0 and args.validation_output is None:
        print("--validation-fraction requires --validation-output", file=sys.stderr)
        return 2
    if args.limit < 0 or args.extra_legal_negatives < 0:
        print("limits must not be negative", file=sys.stderr)
        return 2

    rows = build_rows(args)
    if not rows:
        print("no pairwise rows built", file=sys.stderr)
        return 1
    random.Random(args.seed).shuffle(rows)
    if args.limit > 0:
        rows = rows[: args.limit]
    validation_rows: list[dict[str, str]] = []
    if args.validation_fraction > 0.0:
        validation_count = max(1, int(len(rows) * args.validation_fraction))
        validation_rows = rows[:validation_count]
        rows = rows[validation_count:]
    write_rows(args.output, rows)
    if args.validation_output is not None:
        write_rows(args.validation_output, validation_rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
