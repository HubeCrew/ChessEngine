#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import re
import sys
from dataclasses import dataclass
from pathlib import Path


ID_RE = re.compile(r'\bid\s+"(?P<id>[^"]+)"')
POSTMORTEM_ID_RE = re.compile(r"^postmortem-\d+-g(?P<game>\d+)-p(?P<ply>\d+)$")


@dataclass(frozen=True)
class PostmortemEvent:
    game: int
    ply: int
    severity: int
    categories: frozenset[str]
    reference_avoid: bool


@dataclass(frozen=True)
class EpdPosition:
    identifier: str
    game: int
    ply: int
    line: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract a focused EPD suite from gauntlet_postmortem.py events and positions."
    )
    parser.add_argument("--events", required=True, type=Path, help="events.csv produced by gauntlet_postmortem.py.")
    parser.add_argument("--positions", required=True, type=Path, help="positions.epd produced by gauntlet_postmortem.py.")
    parser.add_argument("--output", required=True, type=Path, help="Output EPD path.")
    parser.add_argument(
        "--category",
        action="append",
        default=[],
        help="Required category. Can be repeated. By default repeated categories match any.",
    )
    parser.add_argument(
        "--match",
        choices=("any", "all"),
        default="any",
        help="How repeated --category filters combine.",
    )
    parser.add_argument(
        "--exclude-category",
        action="append",
        default=[],
        help="Category to exclude. Can be repeated.",
    )
    parser.add_argument(
        "--reference-avoid-only",
        action="store_true",
        help="Keep only events where the reference engine confirmed the played move as avoidable.",
    )
    parser.add_argument("--min-severity", type=int, default=None, help="Keep only events at or above this severity.")
    parser.add_argument("--max-positions", type=int, default=None, help="Maximum positions to write.")
    parser.add_argument(
        "--sort",
        choices=("event-order", "severity-desc"),
        default="event-order",
        help="Output order for matching positions.",
    )
    return parser.parse_args()


def parse_bool(value: str) -> bool:
    return value.strip().lower() in {"1", "true", "yes", "y"}


def parse_categories(value: str) -> frozenset[str]:
    return frozenset(category for category in value.split("|") if category)


def parse_int_field(row: dict[str, str], field_name: str) -> int:
    value = row.get(field_name, "")
    if value == "":
        raise ValueError(f"events CSV row is missing {field_name!r}")
    return int(value)


def load_events(path: Path) -> list[PostmortemEvent]:
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            raise ValueError("events CSV is empty")
        required = {"game", "ply", "severity", "categories", "reference_avoid"}
        missing = required - set(reader.fieldnames)
        if missing:
            raise ValueError("events CSV is missing required columns: " + ", ".join(sorted(missing)))

        events: list[PostmortemEvent] = []
        seen: set[tuple[int, int]] = set()
        for row in reader:
            event = PostmortemEvent(
                game=parse_int_field(row, "game"),
                ply=parse_int_field(row, "ply"),
                severity=parse_int_field(row, "severity"),
                categories=parse_categories(row.get("categories", "")),
                reference_avoid=parse_bool(row.get("reference_avoid", "")),
            )
            key = (event.game, event.ply)
            if key in seen:
                raise ValueError(f"duplicate postmortem event for game {event.game}, ply {event.ply}")
            seen.add(key)
            events.append(event)
    return events


def parse_epd_identifier(line: str, path: Path, line_number: int) -> str:
    match = ID_RE.search(line)
    if match is None:
        raise ValueError(f"{path}:{line_number}: missing EPD id operation")
    return match.group("id")


def parse_postmortem_id(identifier: str, path: Path, line_number: int) -> tuple[int, int]:
    match = POSTMORTEM_ID_RE.match(identifier)
    if match is None:
        raise ValueError(f"{path}:{line_number}: unsupported postmortem id {identifier!r}")
    return int(match.group("game")), int(match.group("ply"))


def load_positions(path: Path) -> dict[tuple[int, int], EpdPosition]:
    positions: dict[tuple[int, int], EpdPosition] = {}
    with path.open(encoding="utf-8") as handle:
        for line_number, raw_line in enumerate(handle, start=1):
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            identifier = parse_epd_identifier(line, path, line_number)
            game, ply = parse_postmortem_id(identifier, path, line_number)
            key = (game, ply)
            if key in positions:
                previous = positions[key].identifier
                raise ValueError(f"duplicate EPD position for game {game}, ply {ply}: {previous}, {identifier}")
            positions[key] = EpdPosition(identifier=identifier, game=game, ply=ply, line=line)
    return positions


def event_matches(
    event: PostmortemEvent,
    required_categories: set[str],
    match_mode: str,
    excluded_categories: set[str],
    reference_avoid_only: bool,
    min_severity: int | None,
) -> bool:
    if reference_avoid_only and not event.reference_avoid:
        return False
    if min_severity is not None and event.severity < min_severity:
        return False
    if excluded_categories & event.categories:
        return False
    if required_categories:
        if match_mode == "all":
            return required_categories <= event.categories
        return bool(required_categories & event.categories)
    return True


def select_events(events: list[PostmortemEvent], args: argparse.Namespace) -> list[PostmortemEvent]:
    required_categories = set(args.category)
    excluded_categories = set(args.exclude_category)
    selected = [
        event
        for event in events
        if event_matches(
            event,
            required_categories,
            args.match,
            excluded_categories,
            args.reference_avoid_only,
            args.min_severity,
        )
    ]
    if args.sort == "severity-desc":
        selected.sort(key=lambda event: (-event.severity, event.game, event.ply))
    if args.max_positions is not None:
        selected = selected[: args.max_positions]
    return selected


def write_suite(path: Path, selected: list[PostmortemEvent], positions: dict[tuple[int, int], EpdPosition]) -> None:
    missing = [event for event in selected if (event.game, event.ply) not in positions]
    if missing:
        details = ", ".join(f"g{event.game}-p{event.ply}" for event in missing[:10])
        if len(missing) > 10:
            details += ", ..."
        raise ValueError(f"positions EPD is missing selected events: {details}")

    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as handle:
        handle.write("# Extracted postmortem diagnostic suite.\n")
        handle.write("# Regenerate with tools/extract_postmortem_suite.py.\n")
        for event in selected:
            handle.write(positions[(event.game, event.ply)].line)
            handle.write("\n")


def main() -> int:
    args = parse_args()
    try:
        events = load_events(args.events)
        positions = load_positions(args.positions)
        selected = select_events(events, args)
        write_suite(args.output, selected, positions)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    print(f"wrote {len(selected)} postmortem positions to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
