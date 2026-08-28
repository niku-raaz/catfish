# Engine testing workspace

Catfish uses **standard UCI** as its single interoperability boundary:

```text
catfish_core
    |
catfish_uci
    ├── Fastchess ── Stockfish / Berserk / Lc0 / Catfish baseline
    ├── Cute Chess ── manual GUI testing and secondary compatibility
    ├── lichess-bot ── Lichess Bot API
    └── analysis harness ── tactical, perft, and position comparisons
```

Do not add Stockfish- or Lichess-specific engine integrations. Lichess is a
playing platform; the clean path is `catfish_uci` → [lichess-bot](https://github.com/lichess-bot-devs/lichess-bot) → Lichess Bot API.

## Layout

```text
tools/engine-testing/
    README.md
    manifests/          # pinned opponent and suite metadata
    configs/            # Fastchess / Cute Chess command templates
    openings/           # small local suites; pin larger books by checksum
    scripts/            # compliance, smoke matches, process UCI tests
    results/            # gitignored PGNs, logs, and summaries
```

## Current capability

Fixed-depth offline matches work today (`go depth N`). Fair time-controlled
tournaments and reliable Lichess play need Phase 2 (SearchLimits, cooperative
cancellation, responsive UCI input, time allocation). See
[docs/ENGINE_TESTING.md](../../docs/ENGINE_TESTING.md).

## Prerequisites

1. Release build of Catfish:

   ```bash
   cmake --preset release
   cmake --build --preset release
   ```

2. [Fastchess](https://github.com/Disservin/fastchess) on `PATH` (or set
   `FASTCHESS`).
3. For Stockfish smoke matches: a separately obtained Stockfish binary
   (`STOCKFISH`) and an opening suite (`OPENINGS`). Prefer the official
   [Stockfish books](https://github.com/official-stockfish/books) collection
   (for example `2moves_v1.epd`). Pin release tags and record SHA-256 in the
   manifest; never rely on an unpinned “latest” download.

Stockfish is GPLv3. Running a locally obtained binary for tests is fine;
redistributing it with Catfish requires the license and corresponding source.

## Quick commands

```bash
# Process-level UCI transcript tests (real catfish_uci executable)
npm run test:uci-process

# Fastchess UCI compliance
npm run test:uci-compliance

# Wiring-only Stockfish smoke (depth 4, OwnBook off, paired openings)
# Requires STOCKFISH and optionally OPENINGS / CATFISH_UCI
npm run test:engine-smoke
```

Environment overrides:

| Variable | Purpose |
| --- | --- |
| `CATFISH_ENGINE_PATH` / `CATFISH_UCI` | Path to `catfish_uci` |
| `FASTCHESS` | Path to the Fastchess binary |
| `STOCKFISH` | Path to a pinned Stockfish binary |
| `OPENINGS` | Absolute path to an EPD/PGN opening suite |
| `ENGINE_TESTING_SEED` | Random seed for opening order (default `1`) |

Results land under `tools/engine-testing/results/` (gitignored).

## Acceptance gates (Phase 1)

- Fastchess `--compliance` passes without warnings.
- Process UCI tests pass against the real executable.
- Smoke match: 40 paired games (`-rounds 20 -repeat`) complete with zero
  crashes, illegal moves, hangs, or malformed UCI output.
- PGNs contain valid results.

Depth equality is **not** strength equality. Depth 4 vs depth 4 is only a
wiring test.
