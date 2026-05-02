#!/usr/bin/env python3

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path


HEADER = "id,theme,depth,bestmove,expected,matched,score_cp,nodes,nps,time_ms,description,qnodes\n"


def write_csv(path: Path, rows: list[str]) -> None:
    path.write_text(HEADER + "".join(rows), encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: compare_benchmark_results_smoke_test.py <compare_benchmark_results.py>", file=sys.stderr)
        return 2

    script = Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as tempdir:
        temp = Path(tempdir)
        baseline = temp / "baseline.csv"
        candidate = temp / "candidate.csv"
        write_csv(
            baseline,
            [
                "a,fork,5,e2e4,e2e4,true,10,100,1000,100,,20\n",
                "b,pin,5,a1a2,h1h8,false,0,200,1000,200,,40\n",
            ],
        )
        write_csv(
            candidate,
            [
                "a,fork,5,e2e3,e2e4,false,10,150,1000,150,,30\n",
                "b,pin,5,h1h8,h1h8,true,0,300,1000,300,,60\n",
            ],
        )

        completed = subprocess.run(
            [sys.executable, str(script), "--baseline", str(baseline), "--candidate", str(candidate)],
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode != 0:
            print(completed.stderr, file=sys.stderr)
            print(completed.stdout, file=sys.stderr)
            return completed.returncode

        expected = [
            "baseline: positions=2 matched=1",
            "candidate: positions=2 matched=1",
            "delta: matched=0",
            "improvements 1: b",
            "regressions 1: a",
            "changed_bestmoves 2",
        ]
        for text in expected:
            if text not in completed.stdout:
                print(f"missing {text!r} in:\n{completed.stdout}", file=sys.stderr)
                return 1

        failed = subprocess.run(
            [
                sys.executable,
                str(script),
                "--baseline",
                str(baseline),
                "--candidate",
                str(candidate),
                "--fail-on-time-regression-pct",
                "10",
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        if failed.returncode != 1:
            print("expected time regression failure", file=sys.stderr)
            print(failed.stdout, file=sys.stderr)
            print(failed.stderr, file=sys.stderr)
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
