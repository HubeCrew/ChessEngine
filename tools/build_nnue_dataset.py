#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import random
import sys
from pathlib import Path
from typing import Iterable

import chess
import chess.engine
import chess.pgn


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build a Stockfish-labeled CSV dataset for NNUE training.")
    parser.add_argument("--epd", action="append", type=Path, default=[], help="EPD suite to include. Can be repeated.")
    parser.add_argument("--pgn-dir", action="append", type=Path, default=[], help="Directory containing PGN files. Can be repeated.")
    parser.add_argument("--random-games", type=int, default=0, help="Number of random legal games to sample.")
    parser.add_argument("--random-plies", type=int, default=80, help="Maximum plies per random game.")
    parser.add_argument("--stockfish", default="/usr/games/stockfish", help="UCI engine used for labels.")
    parser.add_argument("--stockfish-threads", type=int, default=4, help="Threads for Stockfish labeling.")
    parser.add_argument("--stockfish-hash", type=int, default=512, help="Hash MB for Stockfish labeling.")
    parser.add_argument("--depth", type=int, default=10, help="Reference search depth.")
    parser.add_argument("--nodes", type=int, default=0, help="Optional reference node limit instead of depth-only labeling.")
    parser.add_argument("--limit", type=int, default=0, help="Maximum positions to write after deduplication.")
    parser.add_argument("--clip-cp", type=int, default=1500, help="Clip labels to this centipawn magnitude.")
    parser.add_argument("--seed", type=int, default=1, help="Seed for deterministic random-position generation.")
    parser.add_argument("--resume", action="store_true", help="Append missing labels when the output CSV already exists.")
    parser.add_argument("--progress-every", type=int, default=100, help="Progress interval written to stderr.")
    parser.add_argument("--output", required=True, type=Path, help="Output CSV path.")
    return parser.parse_args()


def fen_key(board: chess.Board) -> str:
    return " ".join(board.fen(en_passant="fen").split()[:4])


def epd_positions(path: Path) -> Iterable[tuple[chess.Board, str]]:
    with path.open(encoding="utf-8") as handle:
        for line_number, raw in enumerate(handle, start=1):
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            fields = line.split()
            if len(fields) < 4:
                continue
            yield chess.Board(" ".join(fields[:4]) + " 0 1"), f"{path}:{line_number}"


def pgn_positions(path: Path) -> Iterable[tuple[chess.Board, str]]:
    with path.open(encoding="utf-8", errors="replace") as handle:
        game_index = 0
        while True:
            game = chess.pgn.read_game(handle)
            if game is None:
                break
            game_index += 1
            board = game.board()
            for ply, move in enumerate(game.mainline_moves(), start=1):
                board.push(move)
                yield board.copy(stack=False), f"{path}:game{game_index}:ply{ply}"


def random_positions(games: int, max_plies: int) -> Iterable[tuple[chess.Board, str]]:
    for game in range(1, games + 1):
        board = chess.Board()
        for ply in range(1, max_plies + 1):
            if board.is_game_over(claim_draw=True):
                break
            move = random.choice(list(board.legal_moves))
            board.push(move)
            yield board.copy(stack=False), f"random:game{game}:ply{ply}"


def load_existing_keys(path: Path) -> set[str]:
    if not path.exists():
        return set()
    print(f"[dataset] loading existing labels from {path}", file=sys.stderr, flush=True)
    keys: set[str] = set()
    with path.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            fen = row.get("fen")
            if not fen:
                continue
            try:
                keys.add(fen_key(chess.Board(fen)))
            except ValueError:
                continue
    print(f"[dataset] existing labeled positions={len(keys)}", file=sys.stderr, flush=True)
    return keys


def collect_positions(
    args: argparse.Namespace,
    existing_keys: set[str],
    target_new_positions: int,
) -> list[tuple[chess.Board, str]]:
    seen: set[str] = set(existing_keys)
    collected: list[tuple[chess.Board, str]] = []

    def add_many(items: Iterable[tuple[chess.Board, str]]) -> None:
        for board, source in items:
            if board.is_game_over(claim_draw=True):
                continue
            key = fen_key(board)
            if key in seen:
                continue
            seen.add(key)
            collected.append((board, source))
            if len(collected) % args.progress_every == 0:
                print(
                    f"[dataset] collected={len(collected)}/{target_new_positions or 'all'} latest={source}",
                    file=sys.stderr,
                    flush=True,
                )
            if target_new_positions > 0 and len(collected) >= target_new_positions:
                return

    for path in args.epd:
        print(f"[dataset] scanning epd={path}", file=sys.stderr, flush=True)
        add_many(epd_positions(path))
        if target_new_positions > 0 and len(collected) >= target_new_positions:
            return collected

    for directory in args.pgn_dir:
        print(f"[dataset] scanning pgn_dir={directory}", file=sys.stderr, flush=True)
        for path in sorted(directory.glob("*.pgn")):
            add_many(pgn_positions(path))
            if target_new_positions > 0 and len(collected) >= target_new_positions:
                return collected

    if args.random_games > 0:
        print(
            f"[dataset] generating random_games={args.random_games} random_plies={args.random_plies}",
            file=sys.stderr,
            flush=True,
        )
        add_many(random_positions(args.random_games, args.random_plies))

    return collected[:target_new_positions] if target_new_positions > 0 else collected


def label_position(engine: chess.engine.SimpleEngine, board: chess.Board, args: argparse.Namespace) -> int:
    limit = chess.engine.Limit(depth=args.depth, nodes=args.nodes if args.nodes > 0 else None)
    info = engine.analyse(board, limit)
    score = info["score"].pov(chess.WHITE).score(mate_score=args.clip_cp)
    if score is None:
        return 0
    return max(-args.clip_cp, min(args.clip_cp, int(score)))


def main() -> int:
    args = parse_args()
    if args.progress_every <= 0:
        print("--progress-every must be positive", file=sys.stderr)
        return 2

    random.seed(args.seed)
    existing_keys = load_existing_keys(args.output) if args.resume else set()
    target_new_positions = args.limit
    if args.resume and args.limit > 0:
        target_new_positions = max(0, args.limit - len(existing_keys))
    if target_new_positions == 0 and args.limit > 0:
        print(f"dataset already has at least {args.limit} positions: {args.output}")
        return 0
    print(
        f"[dataset] start output={args.output} target_total={args.limit or 'all'} target_new={target_new_positions or 'all'}",
        file=sys.stderr,
        flush=True,
    )
    positions = collect_positions(args, existing_keys, target_new_positions)
    if not positions:
        print("no new positions collected", file=sys.stderr)
        return 0 if existing_keys else 1

    print(
        f"[dataset] labeling positions={len(positions)} stockfish={args.stockfish} "
        f"threads={max(1, args.stockfish_threads)} hash={max(1, args.stockfish_hash)} "
        f"depth={args.depth} nodes={args.nodes if args.nodes > 0 else 'none'}",
        file=sys.stderr,
        flush=True,
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    append = args.resume and args.output.exists()
    with chess.engine.SimpleEngine.popen_uci(args.stockfish) as engine, args.output.open("a" if append else "w", encoding="utf-8", newline="") as handle:
        engine.configure({"Threads": max(1, args.stockfish_threads), "Hash": max(1, args.stockfish_hash)})
        writer = csv.DictWriter(handle, fieldnames=["fen", "score_cp", "source"])
        if not append:
            writer.writeheader()
        for index, (board, source) in enumerate(positions, start=1):
            score = label_position(engine, board, args)
            writer.writerow({"fen": board.fen(en_passant="fen"), "score_cp": score, "source": source})
            if index % args.progress_every == 0 or index == len(positions):
                print(f"[{index}/{len(positions)}] labeled source={source} score={score}", file=sys.stderr)
                handle.flush()

    verb = "appended" if append else "wrote"
    print(f"{verb} {len(positions)} labeled positions to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
