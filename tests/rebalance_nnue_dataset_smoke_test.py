#!/usr/bin/env python3
from __future__ import annotations

import csv
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

try:
    import chess
except ImportError:
    print("python-chess not installed; skipping NNUE rebalance smoke", file=sys.stderr)
    raise SystemExit(0)


def source_group(source: str) -> str:
    if source.startswith("random:"):
        return "random"
    if ":game" in source and ":ply" in source:
        return "pgn"
    return Path(source.split(":", 1)[0]).name


def unique_positions(count: int) -> list[str]:
    board = chess.Board()
    positions: list[str] = []
    seen: set[str] = set()
    selector = 0
    while len(positions) < count:
        if board.is_game_over(claim_draw=True) or board.fullmove_number > 80:
            board = chess.Board()
            selector += 1
        legal = list(board.legal_moves)
        move = legal[selector % len(legal)]
        selector += 7
        board.push(move)
        fen = board.fen(en_passant="fen")
        key = " ".join(fen.split()[:4])
        if key in seen:
            continue
        seen.add(key)
        positions.append(fen)
    return positions


def write_dataset(path: Path) -> None:
    scores = [0, 100, 250, 500, 1200]
    fens = unique_positions(90)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=["fen", "score_cp", "source"])
        writer.writeheader()
        for index, fen in enumerate(fens):
            if index < 60:
                source = "tactics.epd:1"
            elif index < 75:
                source = f"random:game{index}:ply1"
            else:
                source = f"games/sample.pgn:game1:ply{index}"
            writer.writerow({"fen": fen, "score_cp": scores[index % len(scores)], "source": source})


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: rebalance_nnue_dataset_smoke_test.py <rebalance_script>", file=sys.stderr)
        return 2
    script = Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        raw = root / "raw.csv"
        balanced = root / "balanced.csv"
        report = root / "report.txt"
        write_dataset(raw)
        completed = subprocess.run(
            [
                sys.executable,
                str(script),
                "--input",
                str(raw),
                "--output",
                str(balanced),
                "--report",
                str(report),
                "--limit",
                "30",
                "--seed",
                "7",
                "--max-source-fraction",
                "0.5",
                "--bucket-fractions",
                "equal=0.2,small=0.2,clear=0.2,large=0.2,decisive=0.2",
            ],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        if completed.returncode != 0:
            print(completed.stdout, file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return completed.returncode
        with balanced.open(encoding="utf-8", newline="") as handle:
            rows = list(csv.DictReader(handle))
        if len(rows) != 30:
            print(f"expected 30 balanced rows, got {len(rows)}", file=sys.stderr)
            return 1
        source_counts = Counter(source_group(row["source"]) for row in rows)
        if source_counts["tactics.epd"] > 15:
            print(f"source cap was not enforced: {source_counts}", file=sys.stderr)
            return 1
        report_text = report.read_text(encoding="utf-8")
        if "selected 30" not in report_text or "[output]" not in report_text:
            print(f"unexpected report:\n{report_text}", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
