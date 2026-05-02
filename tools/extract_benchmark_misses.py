#!/usr/bin/env python3
import argparse
import csv
import sys
from pathlib import Path
from typing import TextIO


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract failed chess_bench CSV rows from one or more source EPD suites."
    )
    parser.add_argument(
        "--source",
        action="append",
        required=True,
        type=Path,
        help="Source EPD suite used for the benchmark. Can be repeated.",
    )
    parser.add_argument(
        "--results",
        required=True,
        help="chess_bench --csv output path, or '-' for stdin.",
    )
    parser.add_argument("--output", required=True, type=Path, help="Output EPD path containing only missed positions.")
    return parser.parse_args()


def trim(value: str) -> str:
    return value.strip()


def split_epd_operations(value: str) -> list[str]:
    operations: list[str] = []
    current: list[str] = []
    in_quote = False
    escaped = False

    for ch in value:
        if escaped:
            current.append(ch)
            escaped = False
            continue
        if ch == "\\" and in_quote:
            current.append(ch)
            escaped = True
            continue
        if ch == '"':
            in_quote = not in_quote
            current.append(ch)
            continue
        if ch == ";" and not in_quote:
            operation = trim("".join(current))
            if operation:
                operations.append(operation)
            current.clear()
            continue
        current.append(ch)

    operation = trim("".join(current))
    if operation:
        operations.append(operation)
    return operations


def parse_epd_string(value: str) -> str:
    value = trim(value)
    if len(value) < 2 or value[0] != '"' or value[-1] != '"':
        return value

    result: list[str] = []
    escaped = False
    for ch in value[1:-1]:
        if escaped:
            result.append(ch)
            escaped = False
        elif ch == "\\":
            escaped = True
        else:
            result.append(ch)
    if escaped:
        result.append("\\")
    return "".join(result)


def parse_operations(text: str) -> dict[str, str]:
    operations: dict[str, str] = {}
    for operation in split_epd_operations(text):
        parts = operation.split(maxsplit=1)
        if not parts:
            continue
        opcode = parts[0]
        operand = parts[1] if len(parts) > 1 else ""
        operations[opcode] = trim(operand)
    return operations


def position_id(line: str, source: Path, line_number: int) -> str:
    fields = line.split(maxsplit=4)
    if len(fields) < 4:
        raise ValueError(f"{source}:{line_number}: expected at least four EPD FEN fields")
    operations_text = fields[4] if len(fields) > 4 else ""
    operations = parse_operations(operations_text)
    if "id" in operations:
        return parse_epd_string(operations["id"])
    return f"{source.stem}-{line_number}"


def load_source_positions(sources: list[Path]) -> dict[str, str]:
    positions: dict[str, str] = {}
    for source in sources:
        with source.open(encoding="utf-8") as input_file:
            for line_number, raw_line in enumerate(input_file, start=1):
                line = raw_line.strip()
                if not line or line.startswith("#"):
                    continue
                identifier = position_id(line, source, line_number)
                if identifier in positions:
                    raise ValueError(f"duplicate EPD id: {identifier}")
                positions[identifier] = line
    return positions


def open_results(path: str) -> tuple[TextIO, bool]:
    if path == "-":
        return sys.stdin, False
    return open(path, newline="", encoding="utf-8"), True


def is_miss(row: dict[str, str]) -> bool:
    value = row.get("matched", "").strip().lower()
    return value in {"false", "0", "no", "n"}


def load_missed_ids(results_path: str) -> list[str]:
    input_file, should_close = open_results(results_path)
    try:
        reader = csv.DictReader(input_file)
        if reader.fieldnames is None:
            raise ValueError("results CSV is empty")
        required = {"id", "matched"}
        missing = required - set(reader.fieldnames)
        if missing:
            raise ValueError(f"results CSV is missing required columns: {', '.join(sorted(missing))}")

        missed: list[str] = []
        seen: set[str] = set()
        for row in reader:
            identifier = row.get("id", "")
            if not identifier:
                raise ValueError("results CSV contains a row without an id")
            if identifier in seen:
                raise ValueError(f"duplicate benchmark result id: {identifier}")
            seen.add(identifier)
            if is_miss(row):
                missed.append(identifier)
        return missed
    finally:
        if should_close:
            input_file.close()


def write_misses(output_path: Path, source_positions: dict[str, str], missed_ids: list[str]) -> None:
    missing_sources = [identifier for identifier in missed_ids if identifier not in source_positions]
    if missing_sources:
        raise ValueError("missing source EPD entries for ids: " + ", ".join(missing_sources))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8", newline="\n") as output_file:
        output_file.write("# Extracted chess_bench misses.\n")
        output_file.write("# Regenerate from a CSV run with tools/extract_benchmark_misses.py.\n")
        for identifier in missed_ids:
            output_file.write(source_positions[identifier])
            output_file.write("\n")


def main() -> int:
    args = parse_args()
    try:
        source_positions = load_source_positions(args.source)
        missed_ids = load_missed_ids(args.results)
        write_misses(args.output, source_positions, missed_ids)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    print(f"wrote {len(missed_ids)} missed positions to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
