import csv
import subprocess
import sys


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: bench_smoke_test.py <chess_bench> <bench_epd>", file=sys.stderr)
        return 2

    command = [sys.argv[1], "--suite", "bench", "--depth", "1", "--hash", "1", "--csv"]
    completed = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=10,
    )
    if completed.returncode != 0:
        print(completed.stderr, file=sys.stderr)
        print(completed.stdout, file=sys.stderr)
        return completed.returncode

    rows = list(csv.DictReader(completed.stdout.splitlines()))
    if len(rows) < 5:
        print("expected multiple benchmark rows", file=sys.stderr)
        print(completed.stdout, file=sys.stderr)
        return 1

    required_columns = {
        "id",
        "theme",
        "depth",
        "bestmove",
        "expected",
        "matched",
        "score_cp",
        "nodes",
        "nps",
        "time_ms",
        "description",
    }
    if set(rows[0].keys()) != required_columns:
        print(f"unexpected columns: {rows[0].keys()}", file=sys.stderr)
        return 1

    for row in rows:
        if row["theme"] != "bench":
            print(f"unexpected theme: {row}", file=sys.stderr)
            return 1
        if int(row["depth"]) != 1:
            print(f"depth override failed: {row}", file=sys.stderr)
            return 1
        if int(row["nodes"]) <= 0:
            print(f"expected nodes > 0: {row}", file=sys.stderr)
            return 1
        if row["bestmove"] == "0000":
            print(f"expected non-null bestmove: {row}", file=sys.stderr)
            return 1

    epd_completed = subprocess.run(
        [sys.argv[1], "--suite", "epd", "--epd", sys.argv[2], "--depth", "1", "--hash", "1", "--csv"],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=10,
    )
    if epd_completed.returncode != 0:
        print(epd_completed.stderr, file=sys.stderr)
        print(epd_completed.stdout, file=sys.stderr)
        return epd_completed.returncode

    epd_rows = list(csv.DictReader(epd_completed.stdout.splitlines()))
    if len(epd_rows) < 5:
        print("expected multiple EPD benchmark rows", file=sys.stderr)
        print(epd_completed.stdout, file=sys.stderr)
        return 1
    if set(epd_rows[0].keys()) != required_columns:
        print(f"unexpected EPD columns: {epd_rows[0].keys()}", file=sys.stderr)
        return 1
    if any(row["theme"] != "bench" for row in epd_rows):
        print(f"unexpected EPD themes: {epd_rows}", file=sys.stderr)
        return 1
    if any(int(row["depth"]) != 1 for row in epd_rows):
        print(f"EPD depth override failed: {epd_rows}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
