#!/usr/bin/env python3

from __future__ import annotations

import csv
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: build_stockfish_tuning_suite_smoke_test.py <build_stockfish_tuning_suite.py>", file=sys.stderr)
        return 2

    script = Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as tempdir:
        temp = Path(tempdir)
        events = temp / "events.csv"
        positions = temp / "positions.epd"
        output_dir = temp / "suite"

        events.write_text(
            "\n".join(
                [
                    "severity,categories,game,ply,uci,reference_bestmove,reference_delta_cp,reference_avoid,engine_reference_delta_cp,first_reference_blunder,opening,phase,result,side,fen_before",
                    "500,engine-loss|reference-blunder|played-negative-see|engine-can-avoid-negative-see,1,8,e4d5,e4e5,240,true,190,true,italian,opening,0-1,white,8/8/8/3p4/4P3/8/8/4K2k w - - 0 1",
                    "300,engine-loss|reference-blunder|bad-trade-sequence,2,10,a1a8,a1a7,160,true,-20,false,french,endgame,1-0,white,r6k/8/8/8/8/8/8/R6K w - - 0 1",
                    "250,eval-swing,3,12,b2b7,b2b8,80,true,20,false,sicilian,middlegame,0-1,white,1r5k/1P6/8/8/8/8/8/4K3 w - - 0 1",
                    "",
                ]
            ),
            encoding="utf-8",
        )
        positions.write_text(
            "\n".join(
                [
                    '8/8/8/3p4/4P3/8/8/4K2k w - - am e4d5; id "postmortem-0001-g1-p8";',
                    'r6k/8/8/8/8/8/8/R6K w - - am a1a8; id "postmortem-0002-g2-p10";',
                    "",
                ]
            ),
            encoding="utf-8",
        )

        completed = subprocess.run(
            [
                sys.executable,
                str(script),
                "--events",
                str(events),
                "--positions",
                str(positions),
                "--output-dir",
                str(output_dir),
                "--suite-name",
                "smoke",
                "--min-reference-delta",
                "120",
            ],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=10,
        )
        if completed.returncode != 0:
            print(completed.stdout, file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return completed.returncode
        if "wrote 2 positions" not in completed.stdout:
            print(f"unexpected stdout:\n{completed.stdout}", file=sys.stderr)
            return 1

        all_epd = (output_dir / "smoke.epd").read_text(encoding="utf-8")
        if 'bm e4e5; am e4d5; id "postmortem-0001-g1-p8"' not in all_epd:
            print(f"missing first confirmed position:\n{all_epd}", file=sys.stderr)
            return 1
        if 'bm a1a7; am a1a8; id "postmortem-0002-g2-p10"' not in all_epd:
            print(f"missing second confirmed position:\n{all_epd}", file=sys.stderr)
            return 1
        if "g3-p12" in all_epd or "b2b8" in all_epd:
            print(f"low reference-delta position should not be included:\n{all_epd}", file=sys.stderr)
            return 1

        avoid_epd = (output_dir / "smoke-avoid.epd").read_text(encoding="utf-8")
        if "bm " in avoid_epd:
            print(f"avoid-only suite should not include bm operations:\n{avoid_epd}", file=sys.stderr)
            return 1
        if 'am e4d5; id "postmortem-0001-g1-p8"' not in avoid_epd:
            print(f"avoid-only suite missing avoid move:\n{avoid_epd}", file=sys.stderr)
            return 1

        can_avoid = (output_dir / "smoke-engine-can-avoid.epd").read_text(encoding="utf-8")
        if "postmortem-0001-g1-p8" not in can_avoid or "postmortem-0002-g2-p10" in can_avoid:
            print(f"unexpected engine-can-avoid bucket:\n{can_avoid}", file=sys.stderr)
            return 1
        prefers_played = (output_dir / "smoke-engine-prefers-played.epd").read_text(encoding="utf-8")
        if "postmortem-0002-g2-p10" not in prefers_played or "postmortem-0001-g1-p8" in prefers_played:
            print(f"unexpected engine-prefers-played bucket:\n{prefers_played}", file=sys.stderr)
            return 1

        with (output_dir / "smoke-index.csv").open(newline="", encoding="utf-8") as handle:
            rows = list(csv.DictReader(handle))
        if [row["id"] for row in rows] != ["postmortem-0001-g1-p8", "postmortem-0002-g2-p10"]:
            print(f"unexpected index rows: {rows}", file=sys.stderr)
            return 1
        if rows[0]["primary_bucket"] != "engine-can-avoid":
            print(f"unexpected first primary bucket: {rows[0]}", file=sys.stderr)
            return 1
        if rows[1]["primary_bucket"] != "engine-prefers-played":
            print(f"unexpected second primary bucket: {rows[1]}", file=sys.stderr)
            return 1

        report = (output_dir / "smoke-report.md").read_text(encoding="utf-8")
        for expected in ("Total unique positions: 2", "`engine-can-avoid`: 1", "`engine-prefers-played`: 1"):
            if expected not in report:
                print(f"missing report text {expected!r}:\n{report}", file=sys.stderr)
                return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
