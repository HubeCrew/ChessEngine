#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import IO, Any


SPLIT_SCALE = 1_000_000
PHASE_WEIGHTS = {"n": 1, "b": 1, "r": 2, "q": 4}
VALID_PLACEMENT_CHARS = set("12345678pnbrqkPNBRQK")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Import the Lichess Stockfish evaluation JSONL dump into canonical NNUE CSV splits, "
            "optionally seeding the output with existing canonical datasets first."
        )
    )
    parser.add_argument("--input", required=True, type=Path, help="lichess_db_eval.jsonl or lichess_db_eval.jsonl.zst.")
    parser.add_argument(
        "--seed-dataset",
        action="append",
        type=Path,
        default=[],
        help="Existing canonical CSV with fen,score_cp columns to copy into the output and dedupe against.",
    )
    parser.add_argument("--output-dir", required=True, type=Path, help="Directory for canonical CSV outputs.")
    parser.add_argument("--target-total-rows", type=int, default=10_000_000, help="Target rows including seed datasets.")
    parser.add_argument("--target-new-rows", type=int, default=0, help="Target new Lichess rows. Overrides --target-total-rows.")
    parser.add_argument("--clip-cp", type=int, default=1500, help="Clip labels to this centipawn magnitude.")
    parser.add_argument("--mate-cp", type=int, default=1500, help="Centipawn label used for mate scores before clipping.")
    parser.add_argument("--min-depth", type=int, default=12, help="Skip Lichess eval entries below this depth.")
    parser.add_argument("--validation-fraction", type=float, default=0.05)
    parser.add_argument("--holdout-fraction", type=float, default=0.05)
    parser.add_argument(
        "--stratify",
        choices=("phase-score-sign", "phase-score", "none"),
        default="phase-score-sign",
        help="Balance newly imported rows by phase, score magnitude, and optionally score sign.",
    )
    parser.add_argument(
        "--allow-nonstandard-material",
        action="store_true",
        help="Keep FENs with impossible material, bad king counts, or pawns on back ranks.",
    )
    parser.add_argument("--progress-every", type=int, default=100000)
    return parser.parse_args()


def split_name(key: str, validation_fraction: float, holdout_fraction: float) -> str:
    digest = hashlib.sha256(key.encode("utf-8")).digest()
    value = int.from_bytes(digest[:8], "little") % SPLIT_SCALE
    holdout_cutoff = int(holdout_fraction * SPLIT_SCALE)
    validation_cutoff = holdout_cutoff + int(validation_fraction * SPLIT_SCALE)
    if value < holdout_cutoff:
        return "holdout"
    if value < validation_cutoff:
        return "validation"
    return "train"


def fingerprint(key: str) -> int:
    return int.from_bytes(hashlib.blake2b(key.encode("utf-8"), digest_size=8).digest(), "little")


def output_fen(key: str) -> str:
    parts = key.split()
    return " ".join(parts[:4] + ["0", "1"])


def normalize_fen(raw_fen: str) -> str:
    parts = raw_fen.split()
    if len(parts) < 4:
        raise ValueError("FEN is missing required fields")
    placement, side, castling, ep = parts[:4]
    if side not in ("w", "b"):
        raise ValueError("FEN has invalid side to move")
    if not placement or "/" not in placement:
        raise ValueError("FEN has invalid placement")
    validate_placement_shape(placement)
    return f"{placement} {side} {castling} {ep}"


def validate_placement_shape(placement: str) -> None:
    ranks = placement.split("/")
    if len(ranks) != 8:
        raise ValueError("FEN placement must contain 8 ranks")
    for rank in ranks:
        files = 0
        previous_digit = False
        if not rank:
            raise ValueError("FEN placement contains an empty rank")
        for char in rank:
            if char not in VALID_PLACEMENT_CHARS:
                raise ValueError("FEN placement contains an invalid piece")
            if char.isdigit():
                if previous_digit:
                    raise ValueError("FEN placement contains consecutive digits")
                files += int(char)
                previous_digit = True
            else:
                files += 1
                previous_digit = False
        if files != 8:
            raise ValueError("FEN placement rank does not contain 8 files")


def material_sanity_reason(key: str) -> str | None:
    placement = key.split()[0]
    counts: Counter = Counter(char for char in placement if char.isalpha())
    total_pieces = sum(counts.values())
    white_pieces = sum(counts[piece] for piece in "PNBRQK")
    black_pieces = sum(counts[piece] for piece in "pnbrqk")
    if counts["K"] != 1 or counts["k"] != 1:
        return "king_count"
    if total_pieces > 32:
        return "too_many_pieces"
    if white_pieces > 16 or black_pieces > 16:
        return "too_many_side_pieces"
    if counts["P"] > 8 or counts["p"] > 8:
        return "too_many_pawns"

    ranks = placement.split("/")
    if "P" in ranks[0] or "p" in ranks[0] or "P" in ranks[7] or "p" in ranks[7]:
        return "pawn_on_back_rank"

    white_king = king_square(placement, "K")
    black_king = king_square(placement, "k")
    if white_king is None or black_king is None:
        return "king_count"
    if abs(white_king[0] - black_king[0]) <= 1 and abs(white_king[1] - black_king[1]) <= 1:
        return "adjacent_kings"
    return None


def king_square(placement: str, king: str) -> tuple[int, int] | None:
    for rank_index, rank in enumerate(placement.split("/")):
        file_index = 0
        for char in rank:
            if char.isdigit():
                file_index += int(char)
                continue
            if char == king:
                return file_index, 7 - rank_index
            file_index += 1
    return None


def phase_bucket(placement: str) -> str:
    phase = 0
    for char in placement:
        phase += PHASE_WEIGHTS.get(char.lower(), 0)
    if phase >= 18:
        return "opening_middlegame"
    if phase >= 8:
        return "middlegame_endgame"
    return "endgame"


def score_bucket(score_cp: int) -> str:
    absolute = abs(score_cp)
    if absolute <= 50:
        return "equal"
    if absolute <= 150:
        return "small"
    if absolute <= 400:
        return "clear"
    if absolute <= 1000:
        return "large"
    return "decisive"


def sign_bucket(score_cp: int) -> str:
    return "neg" if score_cp < 0 else "pos"


def bucket_for(key: str, score_cp: int, stratify: str) -> str:
    if stratify == "none":
        return "all"
    placement = key.split()[0]
    bucket = f"{phase_bucket(placement)}:{score_bucket(score_cp)}"
    if stratify == "phase-score-sign":
        bucket += f":{sign_bucket(score_cp)}"
    return bucket


def all_buckets(stratify: str) -> list[str]:
    if stratify == "none":
        return ["all"]
    phases = ("opening_middlegame", "middlegame_endgame", "endgame")
    scores = ("equal", "small", "clear", "large", "decisive")
    if stratify == "phase-score-sign":
        signs = ("neg", "pos")
        return [f"{phase}:{score}:{sign}" for phase in phases for score in scores for sign in signs]
    return [f"{phase}:{score}" for phase in phases for score in scores]


def bucket_limits(total: int, stratify: str) -> dict[str, int]:
    buckets = all_buckets(stratify)
    base = total // len(buckets)
    remainder = total % len(buckets)
    return {bucket: base + (1 if index < remainder else 0) for index, bucket in enumerate(buckets)}


def writer(path: Path) -> tuple[csv.DictWriter, IO[str]]:
    path.parent.mkdir(parents=True, exist_ok=True)
    handle = path.open("w", encoding="utf-8", newline="")
    csv_writer = csv.DictWriter(handle, fieldnames=["fen", "score_cp", "source"])
    csv_writer.writeheader()
    return csv_writer, handle


def open_text_stream(path: Path) -> tuple[IO[str], subprocess.Popen[bytes] | None]:
    if path.suffix != ".zst":
        return path.open(encoding="utf-8"), None
    zstdcat = shutil.which("zstdcat")
    if zstdcat is None:
        zstdcat = shutil.which("pzstd")
    if zstdcat is None:
        try:
            import zstandard as zstd  # type: ignore
        except ImportError as exc:
            raise RuntimeError("reading .zst input requires zstdcat, pzstd, or the Python zstandard package") from exc
        raw = path.open("rb")
        reader = zstd.ZstdDecompressor().stream_reader(raw)
        return io.TextIOWrapper(reader, encoding="utf-8"), None
    command = [zstdcat, str(path)] if Path(zstdcat).name == "zstdcat" else [zstdcat, "-dc", str(path)]
    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    assert process.stdout is not None
    return io.TextIOWrapper(process.stdout, encoding="utf-8"), process


def selected_eval(entry: dict[str, Any], min_depth: int, mate_cp: int, clip_cp: int) -> tuple[int, int, int] | None:
    candidates = []
    for eval_entry in entry.get("evals", []):
        try:
            depth = int(eval_entry.get("depth", 0))
            knodes = int(eval_entry.get("knodes", 0))
            pvs = eval_entry.get("pvs", [])
        except (TypeError, ValueError):
            continue
        if depth < min_depth or not pvs:
            continue
        pv = pvs[0]
        if "cp" in pv:
            try:
                score = int(pv["cp"])
            except (TypeError, ValueError):
                continue
        elif "mate" in pv:
            try:
                mate = int(pv["mate"])
            except (TypeError, ValueError):
                continue
            score = mate_cp if mate > 0 else -mate_cp
        else:
            continue
        score = max(-clip_cp, min(clip_cp, score))
        candidates.append((depth, knodes, score))
    if not candidates:
        return None
    depth, knodes, score = max(candidates, key=lambda item: (item[0], item[1]))
    return score, depth, knodes


def copy_seed_rows(
    args: argparse.Namespace,
    writers: dict[str, csv.DictWriter],
    seen: set[int],
    stats: Counter,
) -> None:
    for dataset in args.seed_dataset:
        with dataset.open(encoding="utf-8", newline="") as handle:
            reader = csv.DictReader(handle)
            for row_index, row in enumerate(reader, start=1):
                stats["seed_input"] += 1
                try:
                    key = normalize_fen(row["fen"])
                    score = max(-args.clip_cp, min(args.clip_cp, int(float(row["score_cp"]))))
                except (KeyError, ValueError):
                    stats["seed_invalid"] += 1
                    continue
                if not args.allow_nonstandard_material:
                    reason = material_sanity_reason(key)
                    if reason is not None:
                        stats["seed_nonstandard_material"] += 1
                        stats[f"seed_nonstandard_{reason}"] += 1
                        continue
                key_hash = fingerprint(key)
                if key_hash in seen:
                    stats["seed_duplicate"] += 1
                    continue
                seen.add(key_hash)
                split = split_name(key, args.validation_fraction, args.holdout_fraction)
                source = row.get("source") or f"{dataset}:{row_index}"
                writers[split].writerow({"fen": output_fen(key), "score_cp": score, "source": source})
                stats[f"written_{split}"] += 1
                stats["seed_written"] += 1


def import_lichess_rows(
    args: argparse.Namespace,
    writers: dict[str, csv.DictWriter],
    seen: set[int],
    stats: Counter,
    target_new_rows: int,
) -> Counter:
    limits = bucket_limits(target_new_rows, args.stratify)
    bucket_counts: Counter = Counter()
    stream, process = open_text_stream(args.input)
    try:
        for line_number, raw in enumerate(stream, start=1):
            stats["lichess_input"] += 1
            if stats["lichess_written"] >= target_new_rows:
                break
            try:
                entry = json.loads(raw)
                key = normalize_fen(str(entry["fen"]))
                picked = selected_eval(entry, args.min_depth, args.mate_cp, args.clip_cp)
            except (KeyError, TypeError, ValueError, json.JSONDecodeError):
                stats["lichess_invalid"] += 1
                continue
            if not args.allow_nonstandard_material:
                reason = material_sanity_reason(key)
                if reason is not None:
                    stats["lichess_nonstandard_material"] += 1
                    stats[f"lichess_nonstandard_{reason}"] += 1
                    continue
            if picked is None:
                stats["lichess_no_usable_eval"] += 1
                continue
            score, depth, knodes = picked
            bucket = bucket_for(key, score, args.stratify)
            if bucket_counts[bucket] >= limits[bucket]:
                stats["lichess_bucket_full"] += 1
                continue
            key_hash = fingerprint(key)
            if key_hash in seen:
                stats["lichess_duplicate"] += 1
                continue
            seen.add(key_hash)
            split = split_name(key, args.validation_fraction, args.holdout_fraction)
            writers[split].writerow(
                {
                    "fen": output_fen(key),
                    "score_cp": score,
                    "source": f"lichess_eval:{line_number}:depth={depth}:knodes={knodes}:bucket={bucket}",
                }
            )
            stats[f"written_{split}"] += 1
            stats["lichess_written"] += 1
            bucket_counts[bucket] += 1
            if args.progress_every > 0 and line_number % args.progress_every == 0:
                print(
                    f"[lichess-import] scanned={line_number} new={stats['lichess_written']}/{target_new_rows} "
                    f"seed={stats['seed_written']} duplicate={stats['lichess_duplicate']} "
                    f"no_eval={stats['lichess_no_usable_eval']} bucket_full={stats['lichess_bucket_full']}",
                    file=sys.stderr,
                    flush=True,
                )
    finally:
        stream.close()
        if process is not None:
            process.wait()
    return bucket_counts


def write_report(path: Path, args: argparse.Namespace, stats: Counter, bucket_counts: Counter, target_new_rows: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write(f"input {args.input}\n")
        for dataset in args.seed_dataset:
            handle.write(f"seed_dataset {dataset}\n")
        handle.write(f"output_dir {args.output_dir}\n")
        handle.write(f"target_total_rows {args.target_total_rows}\n")
        handle.write(f"target_new_rows {target_new_rows}\n")
        handle.write(f"clip_cp {args.clip_cp}\n")
        handle.write(f"mate_cp {args.mate_cp}\n")
        handle.write(f"min_depth {args.min_depth}\n")
        handle.write(f"validation_fraction {args.validation_fraction:.4f}\n")
        handle.write(f"holdout_fraction {args.holdout_fraction:.4f}\n")
        handle.write(f"stratify {args.stratify}\n")
        handle.write(f"allow_nonstandard_material {args.allow_nonstandard_material}\n")
        for key in sorted(stats):
            handle.write(f"{key} {stats[key]}\n")
        for key in sorted(bucket_counts):
            handle.write(f"bucket {key} {bucket_counts[key]}\n")


def validate_args(args: argparse.Namespace) -> str | None:
    if args.clip_cp <= 0:
        return "--clip-cp must be positive"
    if args.mate_cp <= 0:
        return "--mate-cp must be positive"
    if args.min_depth < 0:
        return "--min-depth must not be negative"
    if args.target_total_rows <= 0:
        return "--target-total-rows must be positive"
    if args.target_new_rows < 0:
        return "--target-new-rows must not be negative"
    if args.validation_fraction < 0.0 or args.holdout_fraction < 0.0:
        return "split fractions must not be negative"
    if args.validation_fraction + args.holdout_fraction >= 1.0:
        return "validation and holdout fractions must sum to less than 1"
    if args.progress_every <= 0:
        return "--progress-every must be positive"
    return None


def main() -> int:
    args = parse_args()
    error = validate_args(args)
    if error is not None:
        print(error, file=sys.stderr)
        return 2

    writers: dict[str, csv.DictWriter] = {}
    handles: list[IO[str]] = []
    for name in ("train", "validation", "holdout"):
        out_writer, handle = writer(args.output_dir / f"{name}.csv")
        writers[name] = out_writer
        handles.append(handle)

    stats: Counter = Counter()
    seen: set[int] = set()
    bucket_counts: Counter = Counter()
    try:
        copy_seed_rows(args, writers, seen, stats)
        target_new_rows = args.target_new_rows
        if target_new_rows == 0:
            target_new_rows = max(0, args.target_total_rows - stats["seed_written"])
        print(
            f"[lichess-import] seed={stats['seed_written']} target_new={target_new_rows} "
            f"target_total={stats['seed_written'] + target_new_rows}",
            file=sys.stderr,
            flush=True,
        )
        if target_new_rows > 0:
            bucket_counts = import_lichess_rows(args, writers, seen, stats, target_new_rows)
    finally:
        for handle in handles:
            handle.close()

    write_report(args.output_dir / "import_report.txt", args, stats, bucket_counts, target_new_rows)
    print(
        f"[lichess-import] wrote train={stats['written_train']} validation={stats['written_validation']} "
        f"holdout={stats['written_holdout']} seed={stats['seed_written']} new={stats['lichess_written']}",
        file=sys.stderr,
        flush=True,
    )
    if stats["lichess_written"] < target_new_rows:
        print(
            f"warning: requested {target_new_rows} new rows but only wrote {stats['lichess_written']}",
            file=sys.stderr,
        )
    print(f"wrote canonical Lichess eval dataset to {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
