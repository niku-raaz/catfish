# Engine testing roadmap

Catfish exposes UCI as its **only** interoperability boundary for engines,
GUIs, tournament runners, and online bots. Do not build Stockfish- or
Lichess-specific engine integrations inside Catfish.

```text
catfish_core
    |
catfish_uci
    ├── Fastchess ── Stockfish / Berserk / Lc0 / Catfish baseline
    ├── Cute Chess ── manual GUI / secondary tournament runner
    ├── lichess-bot ── Lichess Bot API
    └── analysis harness ── tactical, perft, position comparisons
```

Lichess is a playing platform, not an engine. The deployment path is
`catfish_uci` → [lichess-bot](https://github.com/lichess-bot-devs/lichess-bot)
→ [Lichess Bot API](https://lichess.org/api#tag/Bot).

Operational scripts and manifests live under
[tools/engine-testing](../tools/engine-testing/README.md).

## What works today

Minimum fixed-depth offline play:

- UCI handshake and readiness
- `position startpos` / `position fen`
- Legal UCI move parsing
- `go depth N`
- `bestmove`, score, depth, nodes, PV
- Configurable Hash, OwnBook, BookFile, SyzygyPath

Important gaps (see `src/uci.cpp` search parsing and
`include/catfish/search.hpp`):

- All `go` fields except `depth` are ignored
- `go wtime ... btime ...` silently defaults to depth 3
- Search is synchronous; `stop` cannot interrupt it
- No `movetime`, clock/increment, node limit, `infinite`, or ponder
- Only one final `info` line is emitted
- Search has no deadline or cancellation token
- The web bridge is fixed-depth and is **not** the tournament boundary

Therefore fixed-depth Fastchess/Cute Chess smoke matches can begin immediately.
Fair timed tournaments and reliable Lichess play wait for Phase 2.

## Tool roles

| Tool | Role | Priority |
| --- | --- | --- |
| [Fastchess](https://github.com/Disservin/fastchess) | Primary match runner, UCI compliance, SPRT, concurrency | First |
| [Stockfish](https://github.com/official-stockfish/Stockfish) | Protocol/reference opponent and ceiling test | First |
| [Cute Chess](https://github.com/cutechess/cutechess) | Manual GUI + independent tournament implementation | Second |
| [lichess-bot](https://github.com/lichess-bot-devs/lichess-bot) | Bridge to Lichess Bot API | After timed UCI |
| [OpenBench](https://github.com/AndyGrant/OpenBench) | Distributed A/B when local compute saturates | Later |
| [Lc0](https://github.com/LeelaChessZero/lc0) | Architecturally different neural opponent | Later |
| [Berserk](https://github.com/jhonnold/berserk) | Additional strong UCI compatibility check | Later |

## Phase 1 — Fixed-depth integration (current)

Estimated effort: 1–2 days.

1. Engine-testing workspace with pinned manifests (done under
   `tools/engine-testing/`).
2. Protocol compliance: `fastchess --compliance ./build/release/catfish_uci`
   and process-level executable tests (`npm run test:uci-process`).
3. Wiring-only Stockfish smoke with OwnBook off, paired openings, depth 4:

   ```bash
   export STOCKFISH=/absolute/path/to/stockfish
   # optional: OPENINGS=/absolute/path/to/2moves_v1.epd
   npm run test:engine-smoke
   ```

Acceptance: compliance clean; 40 paired games; zero crashes/illegal
moves/hangs/malformed UCI; usable PGN annotations.

## Phase 2 — Tournament-grade UCI

Estimated effort: 4–7 days. Critical path.

1. Introduce `SearchLimits` (`depth`, `nodes`, `movetime`, clocks, `infinite`).
2. Cooperative cancellation with soft/hard deadlines; preserve last completed
   iteration; no incomplete exact TT stores; `stop` → `bestmove` within ~250 ms
   under CI load.
3. Separate UCI input from search (worker thread, generation IDs, join before
   mutating state on `position` / `ucinewgame` / replacement `go` / `quit`).
4. Conservative time allocation + `Move Overhead` option.
5. Iterative `info` lines after each completed depth.

Protocol references:

- [UCI specification](https://backscattering.de/chess/uci/)
- [Stockfish UCI documentation](https://official-stockfish.github.io/docs/stockfish-wiki/UCI-Protocol-and-Stockfish-Commands.html)

## Phase 3 — Strength ladder

Do not use full-strength Stockfish win rate as the primary metric.

1. Catfish candidate vs Catfish baseline (primary for search/eval changes)
2. Catfish vs weakened Stockfish (`UCI_LimitStrength` / Skill Level)
3. Catfish vs full Stockfish (ceiling / blunder / protocol)
4. Catfish vs Lc0 / Berserk (diversity)

Match rules: release builds, one thread, equal hash, ponder off, books off
(tournament openings), equivalent or no tablebases, paired color-reversed
openings, fixed suite hash and seed. Never compare depths as Elo.

Statistical policy: tens of games = smoke; hundreds = directional; SPRT for
accept/reject. Example starting SPRT: `elo0=-3`, `elo1=+3`, `alpha=0.05`,
`beta=0.05`. Cap without decision = inconclusive.

## Phase 4 — Lichess

After Phase 2: dedicated never-played account → BOT upgrade (irreversible) →
`bot:play` token in `LICHESS_BOT_TOKEN` (never commit) → lichess-bot worker
with concurrency 1, ponder off, casual only, rapid/classical with increment,
allowlisted opponents → ≥50 clean casual games before rated/bot pools.

## Phase 5 — CI

- Every PR: existing tests, process UCI transcripts, Fastchess compliance,
  a few paired fixed-depth games against a **pinned** opponent (cached).
- Nightly: bench signature, larger corpora, 50–100 paired games, timing stress.
- Weekly / pre-release: candidate vs baseline SPRT + weakened/full gauntlet.

## Risks

- Depth-based “fairness”
- Stockfish saturation (near-100% losses hide gains)
- Opening-book contamination
- Time-loss noise dominating strength signal
- Hardware drift for timed Elo
- Lichess rating as a benchmark
- GPLv3 redistribution obligations if shipping Stockfish binaries
