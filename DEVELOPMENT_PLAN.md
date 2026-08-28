# Development Plan

This plan follows the initial Catfish reference workflow and keeps correctness checks ahead of strength improvements.

## Phase 0: Lock Move Generation

Goal: legal perft must match known reference values.

Tasks:

- Validate starting position perft:
  - depth 1: `20`
  - depth 2: `400`
  - depth 3: `8902`
  - depth 4: `197281`
  - depth 5: `4865609`
- Validate tactical/special-position perft:
  - Kiwipete.
  - Castling edge cases.
  - En passant pin/discovered-check cases.
  - Promotion cases.
- Fix movegen before adding search complexity.

## Phase 1: Evaluation

Goal: static evaluation gives stable, explainable scores.

Status: completed baseline. Catfish now uses tapered middlegame/endgame
scoring, precomputed pawn masks, piece placement, pawn structure, mobility,
king safety, rook files, and an evaluation trace. Automated parameter tuning
and NNUE remain future experiments.

Tasks:

- Material values:
  - pawn: `100`
  - knight: `320`
  - bishop: `330`
  - rook: `500`
  - queen: `900`
- Add and tune:
  - bishop pair.
  - doubled pawns.
  - isolated pawns.
  - passed pawns.
  - piece-square tables.
  - king safety.

## Phase 2: Search

Goal: return legal moves with consistent scores.

Status: completed strength baseline. Quiescence, terminal detection,
mate-distance scoring, PVS, iterative deepening, killer/history ordering, full
PV, and search-local draw handling are implemented. Time management and
interruptible search remain.

Tasks:

- Keep negamax alpha-beta as the baseline.
- Add quiescence search for captures.
- Add move ordering:
  - captures.
  - promotions.
  - checks.
  - killer moves.
  - history heuristic.
- Add iterative deepening.
- Add time management.

## Phase 3: Transposition Tables

Goal: reuse search results using Zobrist keys.

Status: completed. Board keys are incremental and the configurable persistent
table stores depth, score, static evaluation, bound, move, and generation.

Tasks:

- Add fixed-size hash table.
- Store depth, score, bound type, and best move.
- Add replacement policy.
- Validate that scores match alpha-beta without TT.

## Phase 4: UCI

Goal: run Catfish in standard chess GUIs and tournament runners.

Status: fixed-depth baseline completed and powers the local web UI. Phase 1
engine-testing workspace, process-level UCI tests, and Fastchess smoke scripts
are under `tools/engine-testing/`. Time management, cancellation, and iterative
analysis remain Phase 2 — see [docs/ENGINE_TESTING.md](docs/ENGINE_TESTING.md).

Completed:

- Parse: `uci`, `isready`, `ucinewgame`, `position startpos` / `fen`,
  `go depth N`, idle `stop`, `quit`
- Output: `id name Catfish`, options, `uciok`, `readyok`, `info`, `bestmove`
- Process-level executable transcript tests (`npm run test:uci-process`)

Next (tournament-grade UCI):

- `SearchLimits` for depth, nodes, movetime, clocks, infinite
- Cooperative cancellation and responsive UCI input while searching
- Iterative `info` lines and `Move Overhead`
- Then lichess-bot deployment (not custom Lichess code in the Express bridge)

## Phase 5: Strength Work

Goal: improve engine quality without breaking correctness.

Tasks:

- Magic bitboards or optimized attack tables.
- Better evaluation terms.
- Principal variation search.
- Aspiration windows.
- Null move pruning.
- Late move reductions.
- Opening book support.
- Endgame-specific evaluation.

Completed:

- Principal variation search.
- Opening book support with a compact built-in repertoire and external files.
- Optional local Syzygy root probing through Fathom.
- Tapered evaluation and core positional terms.

Next candidates must be accepted through benchmark regression and controlled
self-play:

- Aspiration windows.
- Conservative late-move reductions.
- Null-move pruning with zugzwang safeguards.
- In-search Syzygy WDL probing.
- Offline book generation from a licensed strong-game corpus.
- Automated handcrafted tuning, followed by an NNUE experiment.

## Always-On Checks

- Run CTest before committing.
- Run perft after changing board, movegen, make/unmake, or FEN.
- Compare search scores before and after pruning or table changes.
- Keep modules small and avoid mixing UI/protocol code into the core engine.
