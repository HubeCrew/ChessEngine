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
- `chess_engine`: simple alpha-beta negamax search with quiescence and material evaluation.
- `chess_perft`: command-line perft divide tool.
- `chess_uci`: minimal UCI protocol entrypoint.

## Next Engine Work

The next quality step is to harden strength and speed: transposition table, stronger move ordering, piece-square tables, search instrumentation, and more perft suites before UI work.
