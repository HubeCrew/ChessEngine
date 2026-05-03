import re
import subprocess
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: uci_smoke_test.py <chess_uci>", file=sys.stderr)
        return 2

    commands = "\n".join(
        [
            "uci",
            "setoption name Hash value 2",
            "setoption name SearchExtensions value true",
            "isready",
            "ucinewgame",
            "position startpos moves e2e4 d7d5",
            "eval",
            "position fen 4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1",
            "see e4d5",
            "go depth 3",
            "quit",
            "",
        ]
    )
    clock_commands = "\n".join(
        [
            "uci",
            "isready",
            "ucinewgame",
            "position startpos moves d2d4 d7d5",
            "go wtime 1000 btime 1000 winc 50 binc 50 movestogo 20",
            "quit",
            "",
        ]
    )
    searchmoves_commands = "\n".join(
        [
            "uci",
            "isready",
            "ucinewgame",
            "position startpos",
            "go depth 2 searchmoves e2e4",
            "quit",
            "",
        ]
    )
    completed = subprocess.run(
        [sys.argv[1]],
        input=commands,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=10,
    )

    if completed.returncode != 0:
        print(completed.stderr, file=sys.stderr)
        return completed.returncode

    output = completed.stdout
    required_fragments = [
        "id name ChessEngine 0.1",
        "option name Hash type spin default 64 min 1 max 4096",
        "option name SearchExtensions type check default true",
        "uciok",
        "readyok",
        "info string eval material ",
        "info string see e4d5 100",
        "bestmove ",
    ]
    for fragment in required_fragments:
        if fragment not in output:
            print(f"missing output fragment: {fragment}", file=sys.stderr)
            print(output, file=sys.stderr)
            return 1

    info_pattern = re.compile(
        r"info depth 3 score cp -?\d+ nodes \d+ qnodes \d+ nps \d+ time \d+( pv [a-h][1-8][a-h][1-8][nbrq]?( [a-h][1-8][a-h][1-8][nbrq]?)*)?"
    )
    if info_pattern.search(output) is None:
        print("missing standard depth-3 info output", file=sys.stderr)
        print(output, file=sys.stderr)
        return 1

    if " safe_mobility " not in output or " trade_context " not in output or " total " not in output:
        print("missing eval trace components", file=sys.stderr)
        print(output, file=sys.stderr)
        return 1

    bestmove_match = re.search(r"bestmove ([a-h][1-8][a-h][1-8][nbrq]?|0000)", output)
    if bestmove_match is None:
        print("missing legal-shaped bestmove", file=sys.stderr)
        print(output, file=sys.stderr)
        return 1

    clock_completed = subprocess.run(
        [sys.argv[1]],
        input=clock_commands,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=10,
    )
    if clock_completed.returncode != 0:
        print(clock_completed.stderr, file=sys.stderr)
        return clock_completed.returncode
    if re.search(r"info depth [1-9]\d* score cp -?\d+ nodes \d+ qnodes \d+ nps \d+ time \d+", clock_completed.stdout) is None:
        print("missing time-control info output", file=sys.stderr)
        print(clock_completed.stdout, file=sys.stderr)
        return 1
    if re.search(r"bestmove ([a-h][1-8][a-h][1-8][nbrq]?|0000)", clock_completed.stdout) is None:
        print("missing time-control bestmove", file=sys.stderr)
        print(clock_completed.stdout, file=sys.stderr)
        return 1

    searchmoves_completed = subprocess.run(
        [sys.argv[1]],
        input=searchmoves_commands,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=10,
    )
    if searchmoves_completed.returncode != 0:
        print(searchmoves_completed.stderr, file=sys.stderr)
        return searchmoves_completed.returncode
    if "bestmove e2e4" not in searchmoves_completed.stdout:
        print("searchmoves did not constrain the root move", file=sys.stderr)
        print(searchmoves_completed.stdout, file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
