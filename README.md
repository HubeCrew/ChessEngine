# ChessEngine

Engine-first C++20 chess project. The current foundation is a validated chess rules core, a small search engine, a perft tool, and a minimal UCI-compatible command-line engine.

## Build

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

## Test

```bash
ctest --test-dir build-release --output-on-failure
```

The test suite uses Catch2 and validates FEN round trips, legal move generation, make/unmake restoration, special move edge cases, and standard perft positions.
It also includes engine tests for Zobrist hashing, transposition table behavior, evaluation features, benchmark suites, tactical best moves, principal variation output, and CLI smoke coverage.

## Tools

Run a divided perft from the start position:

```bash
./build-release/chess_perft 3
```

Run the UCI engine manually:

```bash
./build-release/chess_uci
```

Example UCI session:

```text
uci
isready
position startpos moves e2e4 e7e5
go depth 3
go wtime 60000 btime 60000 winc 500 binc 500
quit
```

Run the benchmark and tactical suites:

```bash
./build-release/chess_bench --suite all --hash 64
./build-release/chess_bench --suite bench --depth 3 --csv
./build-release/chess_bench --suite tactics
./build-release/chess_bench --suite bench --depth 5 --disable-null-move
./build-release/chess_bench --suite bench --depth 5 --disable-extensions
./build-release/chess_bench --suite epd --epd data/suites/tactics.epd
./build-release/chess_bench --suite epd --epd data/suites/lichess_tactics_250.epd
./build-release/chess_bench --suite epd --epd data/suites/lichess_tactics_250.epd --progress
```

`chess_bench` reports depth, best move, expected move for tactical cases, score, nodes, qsearch nodes, NPS, and elapsed time. The tactical suite currently contains 50 curated positions across mates, promotions, forks, hanging pieces, winning captures, pawn tactics, checks, and loose-piece tactics. Tactical runs return a non-zero exit code if any expected best move is missed.
`--disable-null-move` is available for A/B checks when measuring the null-move pruning search feature.
`--disable-extensions` is available for A/B checks when measuring bounded search extensions.
`--progress` writes per-position progress updates to stderr, so normal table and CSV output on stdout remain clean.
External suites use an EPD-style format: four FEN fields followed by operations such as `bm` for accepted UCI best moves, `acd` for search depth, `id`, `theme`, `c0`, `hmvc`, and `fmvn`. The checked-in file-backed suites live under `data/suites/`.

UCI `movetime` still takes priority for fixed-time tests. Without `movetime`, the engine allocates a conservative move budget from `wtime`, `btime`, `winc`, `binc`, and optional `movestogo`.

Generate an imported Lichess tactical suite from the official CC0 puzzle database:

```bash
wget -c -O data/raw/lichess_db_puzzle.csv.zst https://database.lichess.org/lichess_db_puzzle.csv.zst
python3 tools/import_lichess_puzzles.py \
  --input data/raw/lichess_db_puzzle.csv.zst \
  --output data/suites/lichess_tactics_250.epd \
  --limit 250 \
  --referee ./build-release/chess_referee
```

The raw database belongs in ignored `data/raw/`. `data/suites/lichess_tactics_250.epd` is an imported, traceable stress suite, not a hand-curated golden suite.

Extract a focused suite from failed benchmark rows:

```bash
./build-release/chess_bench --suite epd --epd data/suites/lichess_tactics_250.epd --csv \
  > data/suites/lichess_tactics_250_results.csv
python3 tools/extract_benchmark_misses.py \
  --source data/suites/lichess_tactics_250.epd \
  --results data/suites/lichess_tactics_250_results.csv \
  --output data/suites/lichess_tactics_misses.epd
./build-release/chess_bench --suite epd --epd data/suites/lichess_tactics_misses.epd --depth 8 --progress
```

The extracted miss suite is useful for diagnosing whether failed tactical positions are fixed by deeper search or need engine changes. Generated CSV result files and extracted miss suites should stay local unless deliberately promoted to curated test data.

Run a local UCI gauntlet:

```bash
./tools/gauntlet.py \
  --engine-a ./build-release/chess_uci \
  --engine-b ./build-release/chess_uci \
  --name-a current \
  --name-b candidate \
  --referee ./build-release/chess_referee \
  --games 20 \
  --movetime 100 \
  --csv
```

Run against Stockfish at its lowest built-in limited Elo:

```bash
./tools/gauntlet.py \
  --engine-a ./build-release/chess_uci \
  --engine-b /usr/games/stockfish \
  --name-a current \
  --name-b stockfish-1320 \
  --referee ./build-release/chess_referee \
  --games 200 \
  --movetime 100 \
  --option-b UCI_LimitStrength=true \
  --option-b UCI_Elo=1320 \
  --csv
```

Run a clock-based gauntlet instead of fixed per-move time:

```bash
./tools/gauntlet.py \
  --engine-a ./build-release/chess_uci \
  --engine-b /usr/games/stockfish \
  --name-a current-release \
  --name-b stockfish-1320 \
  --referee ./build-release/chess_referee \
  --games 200 \
  --time 60000 \
  --increment 500 \
  --moves-to-go 30 \
  --option-b UCI_LimitStrength=true \
  --option-b UCI_Elo=1320 \
  --csv
```

The gauntlet launches both engines as UCI subprocesses, alternates colors, uses a built-in balanced opening suite with color reversal, validates every move through `chess_referee`, writes PGNs to `gauntlet-results/`, and reports score plus a rough Elo difference. It supports either fixed `movetime` or clock-based `wtime/btime/winc/binc` games. Non-clean games, such as crashes, timeouts, protocol failures, or illegal moves, are separated from normal chess results.

Watch a gauntlet live in the browser:

```bash
python3 tools/gauntlet_live_server.py \
  --state runs/gauntlets/current-qsearch-vs-stockfish-1500-fixed/live-state.json \
  --port 8765
```

Open `http://127.0.0.1:8765`, then run `tools/gauntlet.py` with the same `--output-dir`. The runner maintains `live-state.json` after every move, and the viewer polls it to render the current board, clocks, move stream, and match score.

## Architecture

- `chess_core`: board state, bitboards, FEN, legal move generation, make/unmake, perft.
- `chess_engine`: alpha-beta negamax search, quiescence, transposition table support, and static evaluation.
- Search infrastructure: deterministic Zobrist hashing, transposition table, PVS, aspiration windows, SEE-assisted move ordering, killer/history move ordering, LMR, conservative null-move pruning, bounded search extensions, qsearch delta pruning, limited qsearch checks, and standard UCI search info.
- Evaluation: tapered material/PST scoring, mobility, bishop pair, pawn structure, passed pawns, and basic king terms.
- `chess_perft`: command-line perft divide tool.
- `chess_uci`: minimal UCI protocol entrypoint.
- `chess_bench`: repeatable benchmark/tactical harness for measuring strength and speed changes.
- `chess_referee`: machine-readable game adjudicator used by the gauntlet.
- `tools/gauntlet.py`: UCI engine-vs-engine gauntlet with PGN/CSV output and rough Elo reporting.
- `tools/gauntlet_live_server.py`: local browser viewer for following the gauntlet live from `live-state.json`.

## Next Engine Work

The next quality step is benchmark infrastructure: move the tactical suite to a standard EPD-style data file, support larger 200+ position suites without bloating C++ source, and add deeper tactical/stress sets before the next long gauntlet.
