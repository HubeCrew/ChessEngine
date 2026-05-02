import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: extract_benchmark_misses_smoke_test.py <extract_tool>", file=sys.stderr)
        return 2

    tool = Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as tmpdir:
        temp = Path(tmpdir)
        source = temp / "suite.epd"
        results = temp / "results.csv"
        output = temp / "misses.epd"

        source.write_text(
            "\n".join(
                [
                    "# source comment",
                    '8/8/8/8/8/8/8/K6k w - - bm a1a2; id "keep-1"; theme "unit"; acd 1;',
                    '8/8/8/8/8/8/K7/7k w - - bm a2a3; id "drop-1"; theme "unit"; acd 1;',
                    '8/8/8/8/8/K7/8/7k w - - bm a3a4; id "keep-2"; theme "unit"; acd 1;',
                    "",
                ]
            ),
            encoding="utf-8",
        )
        results.write_text(
            "\n".join(
                [
                    "id,theme,depth,bestmove,expected,matched,score_cp,nodes,nps,time_ms,description",
                    '"keep-1","unit",1,"a1b1","a1a2",false,0,1,1,1,""',
                    '"drop-1","unit",1,"a2a3","a2a3",true,0,1,1,1,""',
                    '"keep-2","unit",1,"a3b3","a3a4",false,0,1,1,1,""',
                    "",
                ]
            ),
            encoding="utf-8",
        )

        completed = subprocess.run(
            [sys.executable, str(tool), "--source", str(source), "--results", str(results), "--output", str(output)],
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

        output_text = output.read_text(encoding="utf-8")
        if "keep-1" not in output_text or "keep-2" not in output_text:
            print(f"expected missed ids in output:\n{output_text}", file=sys.stderr)
            return 1
        if "drop-1" in output_text:
            print(f"matched id was incorrectly extracted:\n{output_text}", file=sys.stderr)
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
