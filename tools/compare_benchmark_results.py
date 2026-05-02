#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Compare two chess_bench CSV result files.")
    parser.add_argument("--baseline", required=True, type=Path, help="Baseline chess_bench --csv result file.")
    parser.add_argument("--candidate", required=True, type=Path, help="Candidate chess_bench --csv result file.")
    parser.add_argument(
        "--fail-on-solve-regression",
        action="store_true",
        help="Exit non-zero if the candidate solves fewer positions than the baseline.",
    )
    parser.add_argument(
        "--fail-on-time-regression-pct",
        type=float,
        default=None,
        help="Exit non-zero if candidate total time is more than this percent slower than baseline.",
    )
    return parser.parse_args()


def read_rows(path: Path) -> dict[str, dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as input_file:
        rows = list(csv.DictReader(input_file))
    if not rows:
        raise ValueError(f"empty benchmark CSV: {path}")
    if "id" not in rows[0] or "matched" not in rows[0]:
        raise ValueError(f"not a chess_bench CSV: {path}")
    return {row["id"]: row for row in rows}


def int_field(row: dict[str, str], field: str) -> int:
    value = row.get(field, "")
    return int(value) if value else 0


def matched(row: dict[str, str]) -> bool:
    return row["matched"].lower() == "true"


def totals(rows: dict[str, dict[str, str]]) -> dict[str, int]:
    return {
        "positions": len(rows),
        "matched": sum(1 for row in rows.values() if matched(row)),
        "nodes": sum(int_field(row, "nodes") for row in rows.values()),
        "qnodes": sum(int_field(row, "qnodes") for row in rows.values()),
        "time_ms": sum(int_field(row, "time_ms") for row in rows.values()),
    }


def pct_delta(candidate: int, baseline: int) -> float:
    if baseline == 0:
        return 0.0
    return (candidate - baseline) * 100.0 / baseline


def print_total(label: str, total: dict[str, int]) -> None:
    nps = total["nodes"] * 1000 // max(1, total["time_ms"])
    print(
        f"{label}: positions={total['positions']} matched={total['matched']} "
        f"nodes={total['nodes']} qnodes={total['qnodes']} time_ms={total['time_ms']} nps={nps}"
    )


def main() -> int:
    args = parse_args()
    try:
        baseline = read_rows(args.baseline)
        candidate = read_rows(args.candidate)
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr)
        return 2

    common_ids = sorted(set(baseline) & set(candidate))
    if not common_ids:
        print("no shared position ids", file=sys.stderr)
        return 2

    baseline_common = {position_id: baseline[position_id] for position_id in common_ids}
    candidate_common = {position_id: candidate[position_id] for position_id in common_ids}
    baseline_total = totals(baseline_common)
    candidate_total = totals(candidate_common)

    regressions = [
        position_id
        for position_id in common_ids
        if matched(baseline[position_id]) and not matched(candidate[position_id])
    ]
    improvements = [
        position_id
        for position_id in common_ids
        if not matched(baseline[position_id]) and matched(candidate[position_id])
    ]
    changed_bestmoves = [
        position_id
        for position_id in common_ids
        if baseline[position_id].get("bestmove") != candidate[position_id].get("bestmove")
    ]

    print_total("baseline", baseline_total)
    print_total("candidate", candidate_total)
    print(
        "delta: "
        f"matched={candidate_total['matched'] - baseline_total['matched']} "
        f"nodes_pct={pct_delta(candidate_total['nodes'], baseline_total['nodes']):+.1f}% "
        f"qnodes_pct={pct_delta(candidate_total['qnodes'], baseline_total['qnodes']):+.1f}% "
        f"time_pct={pct_delta(candidate_total['time_ms'], baseline_total['time_ms']):+.1f}%"
    )
    print(f"improvements {len(improvements)}: {' '.join(improvements[:20])}")
    print(f"regressions {len(regressions)}: {' '.join(regressions[:20])}")
    print(f"changed_bestmoves {len(changed_bestmoves)}")

    failed = False
    if args.fail_on_solve_regression and candidate_total["matched"] < baseline_total["matched"]:
        failed = True
    if args.fail_on_time_regression_pct is not None:
        if pct_delta(candidate_total["time_ms"], baseline_total["time_ms"]) > args.fail_on_time_regression_pct:
            failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
