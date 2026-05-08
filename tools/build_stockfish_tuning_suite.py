#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import re
import sys
from dataclasses import dataclass
from pathlib import Path


BUCKET_ORDER = (
    "engine-can-avoid",
    "engine-prefers-played",
    "negative-see",
    "bad-trade-sequence",
    "first-reference-blunder",
    "stockfish-confirmed",
)

BUCKET_DESCRIPTIONS = {
    "engine-can-avoid": "The local search already scores the reference move above the played move when constrained. Prioritize move ordering, LMR, and pruning.",
    "engine-prefers-played": "Stockfish says the played move is avoidable, but the local constrained search does not clearly prefer Stockfish's move. Prioritize eval, horizon, or extension work.",
    "negative-see": "The played move is a negative-SEE move. Prioritize SEE-aware ordering/pruning and compensation checks.",
    "bad-trade-sequence": "The miss is part of a recapture, queen-trade, or bad-trade sequence.",
    "first-reference-blunder": "First Stockfish-confirmed blunder in its game.",
    "stockfish-confirmed": "Stockfish/reference confirmed a better move than the played gauntlet move.",
}

ID_RE = re.compile(r'\bid\s+"(?P<id>[^"]+)"')


@dataclass(frozen=True)
class Event:
    row_index: int
    game: int
    ply: int
    severity: int
    categories: frozenset[str]
    fen: str
    played_move: str
    reference_move: str
    reference_delta: int | None
    reference_avoid: bool
    engine_reference_delta: int | None
    first_reference_blunder: bool
    opening: str
    phase: str
    result: str
    side: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Build Stockfish-reference EPD tuning suites from gauntlet_postmortem.py output. "
            "The generated suites are meant for quick chess_bench runs, not full gauntlets."
        )
    )
    parser.add_argument("--events", required=True, type=Path, help="events.csv produced by gauntlet_postmortem.py.")
    parser.add_argument(
        "--positions",
        type=Path,
        default=None,
        help="Optional positions.epd from the same postmortem. Used only to preserve existing ids when possible.",
    )
    parser.add_argument("--output-dir", required=True, type=Path, help="Directory for EPD suites and reports.")
    parser.add_argument(
        "--suite-name",
        default="stockfish-reference",
        help="Base name for generated files. Default: stockfish-reference.",
    )
    parser.add_argument(
        "--min-reference-delta",
        type=int,
        default=120,
        help="Minimum Stockfish/reference delta in cp for reference-confirmed positions.",
    )
    parser.add_argument("--min-severity", type=int, default=0, help="Minimum postmortem severity to include.")
    parser.add_argument(
        "--max-positions",
        type=int,
        default=160,
        help="Maximum unique positions in the all-position suite after sorting by severity.",
    )
    parser.add_argument(
        "--max-per-bucket",
        type=int,
        default=80,
        help="Maximum positions written to each bucket-specific suite.",
    )
    parser.add_argument(
        "--acd",
        type=int,
        default=5,
        help="EPD acd depth to write when benchmarking without --depth override.",
    )
    parser.add_argument(
        "--include-unconfirmed",
        action="store_true",
        help="Also include severe unconfirmed postmortem events. Confirmed reference positions remain preferred.",
    )
    return parser.parse_args()


def parse_bool(value: str) -> bool:
    return value.strip().lower() in {"1", "true", "yes", "y"}


def parse_int(value: str) -> int | None:
    value = value.strip()
    if not value:
        return None
    try:
        return int(value)
    except ValueError as error:
        raise ValueError(f"invalid integer value {value!r}") from error


def parse_categories(value: str) -> frozenset[str]:
    return frozenset(category for category in value.split("|") if category)


def require_int(row: dict[str, str], field: str) -> int:
    value = parse_int(row.get(field, ""))
    if value is None:
        raise ValueError(f"events CSV row is missing {field!r}")
    return value


def load_events(path: Path) -> list[Event]:
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            raise ValueError("events CSV is empty")
        required = {"game", "ply", "severity", "categories", "uci", "fen_before", "reference_avoid"}
        missing = required - set(reader.fieldnames)
        if missing:
            raise ValueError("events CSV is missing required columns: " + ", ".join(sorted(missing)))

        events: list[Event] = []
        seen: set[tuple[int, int]] = set()
        for row_index, row in enumerate(reader, start=1):
            game = require_int(row, "game")
            ply = require_int(row, "ply")
            key = (game, ply)
            if key in seen:
                raise ValueError(f"duplicate event for game {game}, ply {ply}")
            seen.add(key)
            events.append(
                Event(
                    row_index=row_index,
                    game=game,
                    ply=ply,
                    severity=require_int(row, "severity"),
                    categories=parse_categories(row.get("categories", "")),
                    fen=row.get("fen_before", "").strip(),
                    played_move=row.get("uci", "").strip(),
                    reference_move=row.get("reference_bestmove", "").strip(),
                    reference_delta=parse_int(row.get("reference_delta_cp", "")),
                    reference_avoid=parse_bool(row.get("reference_avoid", "")),
                    engine_reference_delta=parse_int(row.get("engine_reference_delta_cp", "")),
                    first_reference_blunder=parse_bool(row.get("first_reference_blunder", "")),
                    opening=row.get("opening", "").strip(),
                    phase=row.get("phase", "").strip(),
                    result=row.get("result", "").strip(),
                    side=row.get("side", "").strip(),
                )
            )
    return events


def load_position_ids(path: Path | None) -> dict[tuple[int, int], str]:
    if path is None:
        return {}
    ids: dict[tuple[int, int], str] = {}
    with path.open(encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            match = ID_RE.search(line)
            if match is None:
                continue
            identifier = match.group("id")
            id_match = re.match(r"^postmortem-\d+-g(?P<game>\d+)-p(?P<ply>\d+)$", identifier)
            if id_match is None:
                continue
            ids[(int(id_match.group("game")), int(id_match.group("ply")))] = identifier
    return ids


def reference_confirmed(event: Event, min_reference_delta: int) -> bool:
    if not event.reference_avoid or not event.reference_move:
        return False
    return event.reference_delta is not None and event.reference_delta >= min_reference_delta


def classify_event(event: Event, min_reference_delta: int) -> set[str]:
    buckets: set[str] = set()
    if reference_confirmed(event, min_reference_delta):
        buckets.add("stockfish-confirmed")
        if event.engine_reference_delta is not None and event.engine_reference_delta >= 80:
            buckets.add("engine-can-avoid")
        elif event.engine_reference_delta is not None and event.engine_reference_delta <= 0:
            buckets.add("engine-prefers-played")
    if event.first_reference_blunder:
        buckets.add("first-reference-blunder")
    if "played-negative-see" in event.categories:
        buckets.add("negative-see")
    if event.categories & {"bad-trade-sequence", "recapture-trade", "queen-trade-sequence"}:
        buckets.add("bad-trade-sequence")
    return buckets


def primary_bucket(buckets: set[str]) -> str:
    for bucket in BUCKET_ORDER:
        if bucket in buckets:
            return bucket
    return "unconfirmed"


def event_sort_key(event: Event) -> tuple[int, int, int]:
    return (-event.severity, event.game, event.ply)


def select_events(events: list[Event], args: argparse.Namespace) -> list[tuple[Event, set[str]]]:
    selected: list[tuple[Event, set[str]]] = []
    for event in events:
        if event.severity < args.min_severity:
            continue
        if not event.fen or not event.played_move:
            continue
        buckets = classify_event(event, args.min_reference_delta)
        if "stockfish-confirmed" not in buckets and not args.include_unconfirmed:
            continue
        if not buckets and not args.include_unconfirmed:
            continue
        selected.append((event, buckets))

    selected.sort(key=lambda item: event_sort_key(item[0]))
    if args.max_positions is not None:
        selected = selected[: args.max_positions]
    return selected


def epd_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', r"\"") + '"'


def epd_operations(event: Event, buckets: set[str], identifier: str, acd: int, include_reference_best: bool) -> list[str]:
    operations: list[str] = []
    if include_reference_best and event.reference_move:
        operations.append(f"bm {event.reference_move}")
    operations.append(f"am {event.played_move}")
    operations.append(f"id {epd_string(identifier)}")
    operations.append(f"theme {epd_string('stockfish-tuning:' + primary_bucket(buckets))}")
    operations.append(f"acd {acd}")

    fields = event.fen.split()
    if len(fields) >= 6:
        operations.append(f"hmvc {fields[4]}")
        operations.append(f"fmvn {fields[5]}")

    comment_parts = [
        f"g{event.game} p{event.ply}",
        f"severity {event.severity}",
        f"played {event.played_move}",
    ]
    if event.reference_move:
        comment_parts.append(f"reference {event.reference_move}")
    if event.reference_delta is not None:
        comment_parts.append(f"ref_delta {event.reference_delta}")
    if event.engine_reference_delta is not None:
        comment_parts.append(f"engine_ref_delta {event.engine_reference_delta}")
    if event.opening:
        comment_parts.append(f"opening {event.opening}")
    operations.append(f"c0 {epd_string('; '.join(comment_parts))}")

    if event.categories:
        operations.append(f"c1 {epd_string('|'.join(sorted(event.categories)))}")
    return operations


def to_epd_line(event: Event, buckets: set[str], identifier: str, acd: int, include_reference_best: bool) -> str:
    fields = event.fen.split()
    if len(fields) < 4:
        raise ValueError(f"event g{event.game}-p{event.ply} has invalid FEN: {event.fen!r}")
    return " ".join(fields[:4]) + " " + "; ".join(epd_operations(event, buckets, identifier, acd, include_reference_best)) + ";"


def write_epd(path: Path, rows: list[tuple[Event, set[str], str]], acd: int, include_reference_best: bool = True) -> None:
    with path.open("w", encoding="utf-8", newline="\n") as handle:
        handle.write("# Stockfish-reference tuning suite generated from postmortem output.\n")
        handle.write("# Regenerate with tools/build_stockfish_tuning_suite.py.\n")
        if include_reference_best:
            handle.write("# bm is the Stockfish/reference move; am is the played gauntlet move to avoid.\n")
        else:
            handle.write("# Avoid-only suite: am is the played gauntlet move to avoid.\n")
        for event, buckets, identifier in rows:
            handle.write(to_epd_line(event, buckets, identifier, acd, include_reference_best))
            handle.write("\n")


def write_index_csv(path: Path, rows: list[tuple[Event, set[str], str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "id",
                "primary_bucket",
                "buckets",
                "severity",
                "game",
                "ply",
                "played",
                "reference",
                "reference_delta_cp",
                "engine_reference_delta_cp",
                "opening",
                "phase",
                "result",
                "side",
                "categories",
                "fen",
            ]
        )
        for event, buckets, identifier in rows:
            writer.writerow(
                [
                    identifier,
                    primary_bucket(buckets),
                    "|".join(bucket for bucket in BUCKET_ORDER if bucket in buckets),
                    event.severity,
                    event.game,
                    event.ply,
                    event.played_move,
                    event.reference_move,
                    "" if event.reference_delta is None else event.reference_delta,
                    "" if event.engine_reference_delta is None else event.engine_reference_delta,
                    event.opening,
                    event.phase,
                    event.result,
                    event.side,
                    "|".join(sorted(event.categories)),
                    event.fen,
                ]
            )


def write_report(path: Path, rows: list[tuple[Event, set[str], str]], bucket_counts: dict[str, int], suite_name: str) -> None:
    with path.open("w", encoding="utf-8", newline="\n") as handle:
        handle.write(f"# {suite_name} Stockfish Tuning Suite\n\n")
        handle.write(f"Total unique positions: {len(rows)}\n\n")
        handle.write("## Buckets\n\n")
        for bucket in BUCKET_ORDER:
            count = bucket_counts.get(bucket, 0)
            if count == 0:
                continue
            handle.write(f"- `{bucket}`: {count}. {BUCKET_DESCRIPTIONS[bucket]}\n")
        handle.write("\n## Suggested Quick Checks\n\n")
        handle.write("Run the strict generated EPD suite with `chess_bench --suite epd --epd <all.epd> --depth 5 --csv --progress`.\n")
        handle.write("Run `<suite-name>-avoid.epd` when you only want to check that the engine stops replaying the gauntlet blunder.\n")
        handle.write("Use bucket EPDs when changing one search area, for example `engine-can-avoid.epd` after move-ordering or LMR changes.\n")
        handle.write("\n## Highest Severity Positions\n\n")
        for event, buckets, identifier in rows[:20]:
            ref_delta = "" if event.reference_delta is None else f", ref delta {event.reference_delta} cp"
            engine_delta = "" if event.engine_reference_delta is None else f", engine ref delta {event.engine_reference_delta} cp"
            handle.write(
                f"- `{identifier}` `{primary_bucket(buckets)}` severity {event.severity}: "
                f"played `{event.played_move}`, reference `{event.reference_move or '-'}`"
                f"{ref_delta}{engine_delta}\n"
            )


def main() -> int:
    args = parse_args()
    try:
        if args.acd <= 0:
            raise ValueError("--acd must be positive")
        events = load_events(args.events)
        known_ids = load_position_ids(args.positions)
        selected = select_events(events, args)

        rows: list[tuple[Event, set[str], str]] = []
        for index, (event, buckets) in enumerate(selected, start=1):
            identifier = known_ids.get((event.game, event.ply), f"stockfish-tuning-{index:04d}-g{event.game}-p{event.ply}")
            rows.append((event, buckets, identifier))

        args.output_dir.mkdir(parents=True, exist_ok=True)
        all_path = args.output_dir / f"{args.suite_name}.epd"
        write_epd(all_path, rows, args.acd)
        write_epd(args.output_dir / f"{args.suite_name}-avoid.epd", rows, args.acd, include_reference_best=False)

        bucket_counts: dict[str, int] = {}
        for bucket in BUCKET_ORDER:
            bucket_rows = [row for row in rows if bucket in row[1]]
            bucket_counts[bucket] = len(bucket_rows)
            if not bucket_rows:
                continue
            if args.max_per_bucket is not None:
                bucket_rows = bucket_rows[: args.max_per_bucket]
            write_epd(args.output_dir / f"{args.suite_name}-{bucket}.epd", bucket_rows, args.acd)

        write_index_csv(args.output_dir / f"{args.suite_name}-index.csv", rows)
        write_report(args.output_dir / f"{args.suite_name}-report.md", rows, bucket_counts, args.suite_name)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    print(f"wrote {len(rows)} positions to {all_path}")
    for bucket in BUCKET_ORDER:
        count = bucket_counts.get(bucket, 0)
        if count:
            print(f"{bucket}: {count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
