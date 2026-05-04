#!/usr/bin/env python3
import argparse
import csv
import subprocess
import sys
from pathlib import Path
from typing import Iterable, TextIO


DEFAULT_THEMES = {
    "advancedPawn",
    "attraction",
    "clearance",
    "crushing",
    "deflection",
    "discoveredAttack",
    "doubleCheck",
    "fork",
    "hangingPiece",
    "mate",
    "mateIn2",
    "mateIn3",
    "mateIn4",
    "pin",
    "promotion",
    "sacrifice",
    "skewer",
    "trappedPiece",
    "xRayAttack",
}

DEFAULT_EXCLUDED_THEMES = {
    "equality",
    "opening",
    "oneMove",
    "veryLong",
}

THEME_PRIORITY = [
    "mateIn2",
    "mateIn3",
    "mateIn4",
    "mate",
    "fork",
    "pin",
    "skewer",
    "discoveredAttack",
    "doubleCheck",
    "deflection",
    "attraction",
    "sacrifice",
    "promotion",
    "advancedPawn",
    "hangingPiece",
    "trappedPiece",
    "xRayAttack",
    "clearance",
    "crushing",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert filtered Lichess puzzle CSV rows to ChessEngine EPD benchmark positions."
    )
    parser.add_argument("--input", required=True, help="Input .csv, .csv.zst, or '-' for CSV on stdin.")
    parser.add_argument("--output", required=True, type=Path, help="Output EPD path.")
    parser.add_argument("--referee", default="./build/chess_referee", type=Path, help="Path to chess_referee for applying the first Lichess puzzle move.")
    parser.add_argument("--limit", type=int, default=250)
    parser.add_argument("--min-rating", type=int, default=900)
    parser.add_argument("--max-rating", type=int, default=2200)
    parser.add_argument("--max-rating-deviation", type=int, default=90)
    parser.add_argument("--min-popularity", type=int, default=70)
    parser.add_argument("--min-plays", type=int, default=100)
    parser.add_argument("--min-solution-plies", type=int, default=3)
    parser.add_argument("--min-depth", type=int, default=4)
    parser.add_argument("--max-depth", type=int, default=6)
    parser.add_argument("--max-per-theme", type=int, default=25)
    parser.add_argument("--progress-every", type=int, default=5000, help="Input-row progress interval written to stderr.")
    parser.add_argument("--theme", action="append", dest="themes", help="Allowed theme. Can be repeated.")
    parser.add_argument("--exclude-theme", action="append", dest="excluded_themes", help="Excluded theme. Can be repeated.")
    return parser.parse_args()


def open_csv(path: str) -> tuple[TextIO, subprocess.Popen[str] | None]:
    if path == "-":
        return sys.stdin, None
    if path.endswith(".zst"):
        process = subprocess.Popen(
            ["zstdcat", path],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if process.stdout is None:
            raise RuntimeError("failed to open zstdcat stdout")
        return process.stdout, process
    return open(path, newline="", encoding="utf-8"), None


def parse_int(value: str) -> int:
    return int(value.strip())


def split_themes(value: str) -> set[str]:
    return {theme for theme in value.split() if theme}


def primary_theme(themes: set[str]) -> str:
    for theme in THEME_PRIORITY:
        if theme in themes:
            return theme
    return sorted(themes)[0] if themes else "lichess"


def epd_quote(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def referee_fen(referee: Path, fen: str, move: str) -> str:
    completed = subprocess.run(
        [str(referee), "--fen", fen, "--moves", move],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=5,
    )
    if completed.returncode != 0:
        raise ValueError(f"referee rejected puzzle setup move {move}: {completed.stdout}{completed.stderr}")
    for line in completed.stdout.splitlines():
        if line.startswith("fen="):
            return line.removeprefix("fen=")
    raise ValueError(f"referee did not return a FEN for setup move {move}")


def epd_line(row: dict[str, str], theme: str, depth: int, referee: Path) -> str:
    moves = row["Moves"].split()
    if len(moves) < 2:
        raise ValueError("expected at least one setup move and one solution move")

    puzzle_fen = referee_fen(referee, row["FEN"], moves[0])
    fen_fields = puzzle_fen.split()
    if len(fen_fields) != 6:
        raise ValueError(f"expected full FEN, got: {puzzle_fen}")

    solution_moves = moves[1:]
    best_move = solution_moves[0]
    puzzle_id = row["PuzzleId"]
    rating = row["Rating"]
    rating_deviation = row["RatingDeviation"]
    popularity = row["Popularity"]
    plays = row["NbPlays"]
    themes = row["Themes"]
    url = row["GameUrl"]
    description = (
        f"Lichess puzzle {puzzle_id}, rating {rating}, themes {themes}, "
        f"popularity {popularity}, plays {plays}"
    )

    return (
        f"{fen_fields[0]} {fen_fields[1]} {fen_fields[2]} {fen_fields[3]} "
        f"bm {best_move}; "
        f"id {epd_quote('lichess-' + puzzle_id)}; "
        f"theme {epd_quote(theme)}; "
        f"acd {depth}; "
        f"hmvc {fen_fields[4]}; "
        f"fmvn {fen_fields[5]}; "
        f"pv {epd_quote(' '.join(solution_moves))}; "
        f"c0 {epd_quote(description)}; "
        f"c1 {epd_quote(url)}; "
        f"rating {rating}; "
        f"rd {rating_deviation};"
    )


def keep_row(
    row: dict[str, str],
    allowed_themes: set[str],
    excluded_themes: set[str],
    args: argparse.Namespace,
) -> tuple[bool, set[str]]:
    try:
        rating = parse_int(row["Rating"])
        deviation = parse_int(row["RatingDeviation"])
        popularity = parse_int(row["Popularity"])
        plays = parse_int(row["NbPlays"])
    except (KeyError, ValueError):
        return False, set()

    moves = row.get("Moves", "").split()
    themes = split_themes(row.get("Themes", ""))
    if not moves or len(moves) < args.min_solution_plies:
        return False, themes
    if rating < args.min_rating or rating > args.max_rating:
        return False, themes
    if deviation > args.max_rating_deviation:
        return False, themes
    if popularity < args.min_popularity:
        return False, themes
    if plays < args.min_plays:
        return False, themes
    if not (themes & allowed_themes):
        return False, themes
    if themes & excluded_themes:
        return False, themes
    return True, themes


def imported_positions(rows: Iterable[dict[str, str]], args: argparse.Namespace) -> list[str]:
    allowed_themes = set(args.themes) if args.themes else DEFAULT_THEMES
    excluded_themes = set(args.excluded_themes) if args.excluded_themes else DEFAULT_EXCLUDED_THEMES
    per_theme: dict[str, int] = {}
    lines: list[str] = []
    scanned = 0
    kept = 0
    rejected = 0
    invalid = 0

    for row in rows:
        scanned += 1
        keep, themes = keep_row(row, allowed_themes, excluded_themes, args)
        if not keep:
            rejected += 1
            if scanned % args.progress_every == 0:
                print(
                    f"[import] scanned={scanned} kept={kept}/{args.limit} rejected={rejected} invalid={invalid}",
                    file=sys.stderr,
                    flush=True,
                )
            continue

        theme = primary_theme(themes & allowed_themes)
        if per_theme.get(theme, 0) >= args.max_per_theme:
            rejected += 1
            if scanned % args.progress_every == 0:
                print(
                    f"[import] scanned={scanned} kept={kept}/{args.limit} rejected={rejected} invalid={invalid}",
                    file=sys.stderr,
                    flush=True,
                )
            continue

        depth = max(args.min_depth, min(args.max_depth, len(row["Moves"].split()) + 1))
        try:
            line = epd_line(row, theme, depth, args.referee)
        except (KeyError, ValueError):
            invalid += 1
            if scanned % args.progress_every == 0:
                print(
                    f"[import] scanned={scanned} kept={kept}/{args.limit} rejected={rejected} invalid={invalid}",
                    file=sys.stderr,
                    flush=True,
                )
            continue

        lines.append(line)
        kept += 1
        per_theme[theme] = per_theme.get(theme, 0) + 1
        if kept % max(1, min(args.progress_every, 1000)) == 0:
            print(
                f"[import] kept={kept}/{args.limit} scanned={scanned} theme={theme}",
                file=sys.stderr,
                flush=True,
            )
        if len(lines) >= args.limit:
            break

    print(
        f"[import] done scanned={scanned} kept={kept} rejected={rejected} invalid={invalid}",
        file=sys.stderr,
        flush=True,
    )
    return lines


def main() -> int:
    args = parse_args()
    if args.limit <= 0:
        raise SystemExit("--limit must be positive")
    if args.max_per_theme <= 0:
        raise SystemExit("--max-per-theme must be positive")
    if args.min_depth <= 0 or args.max_depth < args.min_depth:
        raise SystemExit("invalid depth range")
    if args.progress_every <= 0:
        raise SystemExit("--progress-every must be positive")

    print(
        f"[import] start input={args.input} output={args.output} limit={args.limit}",
        file=sys.stderr,
        flush=True,
    )
    input_file, process = open_csv(args.input)
    try:
        reader = csv.DictReader(input_file)
        lines = imported_positions(reader, args)
    finally:
        if input_file is not sys.stdin:
            input_file.close()
        if process is not None:
            process.terminate()
            process.wait(timeout=5)

    if len(lines) < args.limit:
        print(f"warning: only imported {len(lines)} positions out of requested {args.limit}", file=sys.stderr)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    print(f"[import] writing {len(lines)} positions to {args.output}", file=sys.stderr, flush=True)
    with args.output.open("w", encoding="utf-8", newline="\n") as output:
        output.write("# Imported from the official Lichess puzzle database (CC0).\n")
        output.write("# Source: https://database.lichess.org/#puzzles\n")
        output.write("# These positions are real imported puzzle data, not hand-curated engine tests.\n")
        for line in lines:
            output.write(line)
            output.write("\n")

    print(f"wrote {len(lines)} positions to {args.output}")
    return 0 if lines else 1


if __name__ == "__main__":
    raise SystemExit(main())
