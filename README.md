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
It also includes engine tests for Zobrist hashing, transposition table behavior, evaluation features, principal variation output, and UCI smoke coverage.

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

## Architecture

- `chess_core`: board state, bitboards, FEN, legal move generation, make/unmake, perft.
- `chess_engine`: alpha-beta negamax search, quiescence, transposition table support, and static evaluation.
- Search infrastructure: deterministic Zobrist hashing, transposition table, killer/history move ordering, and standard UCI search info.
- Evaluation: tapered material/PST scoring, mobility, bishop pair, pawn structure, passed pawns, and basic king terms.
- `chess_perft`: command-line perft divide tool.
- `chess_uci`: minimal UCI protocol entrypoint.

## Next Engine Work

The next quality step is measurement and tuning: benchmark positions, tactical suites, safer quiescence pruning, and controlled value tuning before UI work.
