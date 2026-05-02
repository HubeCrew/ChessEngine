# ChessEngine

Engine-first C++20 chess project. The current foundation is a validated chess rules core, a small search engine, a perft tool, and a minimal UCI-compatible command-line engine.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

The test suite uses Catch2 and validates FEN round trips, legal move generation, make/unmake restoration, special move edge cases, and standard perft positions.
It also includes engine tests for Zobrist hashing, transposition table behavior, evaluation features, benchmark suites, tactical best moves, principal variation output, and CLI smoke coverage.

## Tools

Run a divided perft from the start position:

```bash
./build/chess_perft 3
```

Run the UCI engine manually:

```bash
./build/chess_uci
```

Example UCI session:

```text
uci
isready
position startpos moves e2e4 e7e5
go depth 3
quit
```

Run the benchmark and tactical suites:

```bash
./build/chess_bench --suite all --hash 64
./build/chess_bench --suite bench --depth 3 --csv
./build/chess_bench --suite tactics
```

`chess_bench` reports depth, best move, expected move for tactical cases, score, nodes, NPS, and elapsed time. The tactical suite currently contains 50 curated positions across mates, promotions, forks, hanging pieces, winning captures, pawn tactics, checks, and loose-piece tactics. Tactical runs return a non-zero exit code if any expected best move is missed.

## Architecture

- `chess_core`: board state, bitboards, FEN, legal move generation, make/unmake, perft.
- `chess_engine`: alpha-beta negamax search, quiescence, transposition table support, and static evaluation.
- Search infrastructure: deterministic Zobrist hashing, transposition table, killer/history move ordering, and standard UCI search info.
- Evaluation: tapered material/PST scoring, mobility, bishop pair, pawn structure, passed pawns, and basic king terms.
- `chess_perft`: command-line perft divide tool.
- `chess_uci`: minimal UCI protocol entrypoint.
- `chess_bench`: repeatable benchmark/tactical harness for measuring strength and speed changes.

## Next Engine Work

The next quality step is controlled search improvement: safer quiescence pruning, aspiration windows, and null-move/LMR experiments measured against `chess_bench`.
