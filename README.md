# ChessEngine

Engine-first C++20 chess project. The current foundation is a validated chess rules core, a small search engine, a perft tool, and a minimal UCI-compatible command-line engine.

The project engineering approach is documented in [docs/ENGINEERING_APPROACH.md](docs/ENGINEERING_APPROACH.md). Future strength work should preserve that bigger-picture workflow: diagnose repeated failure classes first, keep tactical strength as a guardrail, and avoid narrow fixes that only patch one benchmark.

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
./build-release/chess_bench --suite epd --epd data/suites/strategy.epd --progress
./build-release/chess_bench --suite epd --epd data/suites/lichess_tactics_250.epd
./build-release/chess_bench --suite epd --epd data/suites/lichess_tactics_250.epd --progress
```

`chess_bench` reports depth, best move, expected move for tactical cases, score, nodes, qsearch nodes, NPS, and elapsed time. The tactical suite currently contains 50 curated positions across mates, promotions, forks, hanging pieces, winning captures, pawn tactics, checks, and loose-piece tactics. Tactical runs return a non-zero exit code if any expected best move is missed.
`data/suites/strategy.epd` is a separate aspirational strategic suite for quieter positional choices such as central breaks, outposts, castling, open-file play, space, and preserving attacking potential. It is useful for diagnosis and tuning; unlike the tactical smoke suite, it is not yet a required no-regression gate because the current engine still misses many of these long-term choices.
`--disable-null-move` is available for A/B checks when measuring the null-move pruning search feature.
`--disable-extensions` is available for A/B checks when measuring bounded search extensions.
`--progress` writes per-position progress updates to stderr, so normal table and CSV output on stdout remain clean.
External suites use an EPD-style format: four FEN fields followed by operations such as `bm` for accepted UCI best moves, `am` for UCI moves that must be avoided, `acd` for search depth, `id`, `theme`, `c0`, `hmvc`, and `fmvn`. A position with only `am` passes when the engine avoids every listed move. The checked-in file-backed suites live under `data/suites/`.

UCI `movetime` still takes priority for fixed-time tests. Without `movetime`, the engine allocates a conservative move budget from `wtime`, `btime`, `winc`, `binc`, and optional `movestogo`.
The UCI engine also supports standard root constraints with `searchmoves`, which is useful when comparing candidate moves from a postmortem:

```text
position fen r1bq1k1r/1p4p1/p3p3/2p2p1p/4Bb2/2NP1N2/PPP2PPP/R2QR1K1 w - - 0 14
go depth 5 searchmoves e4b7
go depth 5 searchmoves g2g3
```

Inspect the evaluation trace for the current UCI position:

```text
position startpos moves e2e4 e7e5 g1f3
eval
```

The custom `eval` command prints white-perspective components for material, piece-square tables, mobility, safe mobility, king safety, threats, pawn structure, outposts, rook files, space, center control, bishop quality, pawn dynamics, development, trade context, classical total, NNUE total when loaded, selected evaluator, and total score.
The custom `see <uci-move>` command prints static exchange evaluation for a legal move in the current position, which is useful for checking whether a capture is tactically profitable before deeper search context is considered.

Run with an exported NNUE network:

```text
./build-release/chess_uci
uci
setoption name NnueFile value runs/nnue/current.nnue
setoption name EvalType value nnue
isready
position startpos
go depth 4
```

`EvalType` supports `classical`, `nnue`, and `hybrid`. Classical remains the default. If `nnue` or `hybrid` is selected without a successfully loaded network, the engine falls back to the classical evaluator.

Create and export a first large local NNUE model:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements-tools.txt

python3 tools/import_lichess_puzzles.py \
  --input data/raw/lichess_db_puzzle.csv.zst \
  --output runs/nnue/lichess_tactics_source_200k.epd \
  --limit 200000 \
  --max-per-theme 20000 \
  --referee ./build-release/chess_referee

python3 tools/build_nnue_dataset.py \
  --epd runs/nnue/lichess_tactics_source_200k.epd \
  --epd data/suites/lichess_tactics_250.epd \
  --epd data/suites/strategy.epd \
  --pgn-dir runs/gauntlets/current-eval-vs-stockfish-1500-smoke \
  --stockfish /usr/games/stockfish \
  --stockfish-threads 4 \
  --stockfish-hash 512 \
  --nodes 8000 \
  --limit 200000 \
  --resume \
  --progress-every 500 \
  --output runs/nnue/dataset.csv

python3 tools/analyze_nnue_dataset.py \
  --dataset runs/nnue/dataset.csv

python3 tools/rebalance_nnue_dataset.py \
  --input runs/nnue/dataset.csv \
  --output runs/nnue/dataset_balanced.csv \
  --limit 160000 \
  --seed 1

python3 tools/analyze_nnue_dataset.py \
  --dataset runs/nnue/dataset_balanced.csv

python3 tools/train_nnue.py \
  --dataset runs/nnue/dataset_balanced.csv \
  --output runs/nnue/current.pt \
  --epochs 12 \
  --batch-size 2048 \
  --hidden-size 256

python3 tools/export_nnue.py \
  --checkpoint runs/nnue/current.pt \
  --output runs/nnue/current.nnue

python3 tools/check_nnue_parity.py \
  --checkpoint runs/nnue/current.pt \
  --nnue runs/nnue/current.nnue \
  --engine ./build-release/chess_uci \
  --dataset runs/nnue/dataset_balanced.csv \
  --limit 256

./build-release/chess_bench \
  --suite epd \
  --epd data/suites/lichess_tactics_250.epd \
  --eval-type nnue \
  --nnue runs/nnue/current.nnue \
  --progress
```

The trainer uses CUDA automatically when PyTorch detects a GPU and falls back to CPU otherwise. The exported `.nnue` file is the only artifact needed by the C++ engine at runtime.

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

Install optional Python tooling dependencies in a local venv:

```bash
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-tools.txt
```

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

Analyze a completed gauntlet after the PGNs are written:

```bash
.venv/bin/python tools/gauntlet_postmortem.py \
  --pgn-dir runs/gauntlets/current-eval-vs-stockfish-1500-smoke \
  --engine ./build-release/chess_uci \
  --engine-name current-eval \
  --output-dir runs/postmortems/current-eval-vs-stockfish-1500 \
  --max-events 80 \
  --engine-depth 1 \
  --reference-engine /usr/games/stockfish \
  --reference-depth 12 \
  --reference-confirm-depth 14 \
  --reference-min-delta-cp 120
```

The postmortem writes `report.md`, `events.csv`, `postmortem.json`, and `positions.epd`. It uses `python-chess` for PGN/FEN reconstruction and the engine's own `eval` command for trace components, then flags eval swings, loss-context moves, equal trades, queen trades, opponent recaptures, and bad trade sequences. When `--engine-depth` is set, it also uses UCI `searchmoves` to record constrained scores and principal variations for the played move and, when available, the reference move, plus the engine's `see` score for each candidate. `positions.epd` records the gauntlet move as `am` when there is no reference engine, or when the reference engine's best move is clearly better by `--reference-min-delta-cp` and remains better at the optional confirmation depth. If the reference engine agrees with the gauntlet move, that move is recorded as `bm`; otherwise the reference move is kept as a comment so the regression suite tests blunder avoidance rather than exact Stockfish imitation.

Extract a focused diagnostic suite from a postmortem category:

```bash
python3 tools/extract_postmortem_suite.py \
  --events runs/postmortems/current-eval-vs-stockfish-1500/events.csv \
  --positions runs/postmortems/current-eval-vs-stockfish-1500/positions.epd \
  --output /tmp/negative_see_avoid.epd \
  --category played-negative-see \
  --reference-avoid-only \
  --sort severity-desc
./build-release/chess_bench --suite epd --epd /tmp/negative_see_avoid.epd --progress
```

This is the preferred way to turn repeated gauntlet failure classes into cheap local experiments. Keep generated focus suites outside the repo unless they are deliberately promoted to stable test data.

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
- Evaluation: tapered classical evaluation plus an opt-in HalfKP-style NNUE shadow evaluator with dependency-free C++ inference.
- `chess_perft`: command-line perft divide tool.
- `chess_uci`: minimal UCI protocol entrypoint.
- `chess_bench`: repeatable benchmark/tactical harness for measuring strength and speed changes.
- `chess_referee`: machine-readable game adjudicator used by the gauntlet.
- `tools/gauntlet.py`: UCI engine-vs-engine gauntlet with PGN/CSV output and rough Elo reporting.
- `tools/gauntlet_postmortem.py`: offline gauntlet analyzer for eval-trace swings, losses, equal trades, queen trades, recaptures, and EPD extraction.
- `tools/extract_postmortem_suite.py`: focused EPD extractor for postmortem categories such as negative-SEE captures or avoid-confirmed blunders.
- `tools/gauntlet_live_server.py`: local browser viewer for following the gauntlet live from `live-state.json`.

## Next Engine Work

The next quality step is evidence-driven evaluation work: use the postmortem category extractor to isolate repeated failure classes, test candidate heuristics against focused suites first, and keep the 250-position tactical suite as a hard guardrail before any long gauntlet.
