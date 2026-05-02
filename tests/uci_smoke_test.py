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
            "isready",
            "ucinewgame",
            "position startpos moves e2e4 e7e5",
            "go depth 3",
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
        "uciok",
        "readyok",
        "bestmove ",
    ]
    for fragment in required_fragments:
        if fragment not in output:
            print(f"missing output fragment: {fragment}", file=sys.stderr)
            print(output, file=sys.stderr)
            return 1

    info_pattern = re.compile(
        r"info depth 3 score cp -?\d+ nodes \d+ nps \d+ time \d+( pv [a-h][1-8][a-h][1-8][nbrq]?( [a-h][1-8][a-h][1-8][nbrq]?)*)?"
    )
    if info_pattern.search(output) is None:
        print("missing standard depth-3 info output", file=sys.stderr)
        print(output, file=sys.stderr)
        return 1

    bestmove_match = re.search(r"bestmove ([a-h][1-8][a-h][1-8][nbrq]?|0000)", output)
    if bestmove_match is None:
        print("missing legal-shaped bestmove", file=sys.stderr)
        print(output, file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

