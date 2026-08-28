# Catfish

Catfish is a C++20 bitboard chess engine with a complete local web application.
Play from either side in a responsive React board while a typed Node bridge
keeps a persistent Catfish UCI process responsible for every computer move.

## Features

- Legal move generation with castling, en passant, and all promotions.
- FEN parsing, perft validation, tapered evaluation, and quiescent PVS search.
- Iterative deepening, incremental Zobrist keys, repetition-aware search, and
  a configurable transposition table.
- A legal-move-validated built-in opening repertoire plus external book files.
- Optional local Syzygy DTZ probing through Fathom.
- Standard UCI executable for Catfish and third-party chess GUIs.
- Click-to-move and drag-and-drop browser board.
- Human versus Catfish as White, Black, or a random side.
- SAN move history, FEN import/export, and PGN export.
- Check, checkmate, stalemate, repetition, fifty-move, and material status in
  the UI.
- Fixed-depth engine strength, White-perspective evaluation, node/hash data,
  move source, and a complete best line.
- Safe full-turn undo, board flip, resignation, and recoverable engine errors.
- Desktop and mobile layouts with keyboard-accessible controls.

## Prerequisites

- A C++20 compiler
- CMake 3.20 or newer
- Node.js 20.19 or newer
- npm

The fallback `Makefile` can build the engine when CMake is unavailable.

## Run the complete application

```bash
npm install
npm run build:engine
npm run dev
```

Open `http://127.0.0.1:5173`.

`npm run dev` starts Vite on port 5173 and the local Catfish bridge on port
8787. Vite proxies `/api` requests to the bridge.

## Production build

```bash
npm install
npm run build
npm start
```

Open `http://127.0.0.1:8787`. The production bridge serves the files generated
in `dist-web/`.

## Deploy

Catfish ships as one production container containing the optimized C++ engine,
compiled Node bridge, and static browser application:

```bash
docker compose up --build
```

Open `http://127.0.0.1:8787` and verify:

```bash
curl http://127.0.0.1:8787/api/health
```

[Render](https://render.com/) is the recommended first deployment target. The
root [render.yaml](render.yaml) creates a Singapore Docker web service with an
engine-aware health check and deploys only after GitHub CI passes. See the
[complete deployment guide](docs/DEPLOYMENT.md) for exact Render steps,
custom-domain setup, Railway/Fly.io alternatives, scaling, and troubleshooting.

## Engine-only build and run

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

CLI examples:

```bash
./build/debug/catfish board
./build/debug/catfish eval
./build/debug/catfish evaltrace
./build/debug/catfish perft 3
./build/debug/catfish search 3
./build/debug/catfish bench 4
./build/debug/catfish_perft 3
```

UCI example:

```bash
printf 'uci\nisready\nposition startpos moves e2e4\ngo depth 3\nquit\n' \
  | ./build/debug/catfish_uci
```

Catfish advertises `Hash`, `Clear Hash`, `OwnBook`, `BookFile`, and
`SyzygyPath` UCI options. The built-in book is enabled by default. Disable it
when benchmarking search:

```text
setoption name OwnBook value false
```

External opening books use UTF-8 tab-separated lines:

```text
40	Sicilian Defense	e2e4 c7c5 g1f3 d7d6 d2d4
```

The fields are positive weight, display name, and a legal UCI move sequence
from the starting position. A file is parsed transactionally before replacing
the active book.

## Optional Syzygy tablebases

Catfish's normal build has no tablebase dependency. To enable local probing,
obtain the MIT-licensed [Fathom](https://github.com/jdart1/Fathom) sources and
configure:

```bash
cmake -S . -B build/syzygy \
  -DCMAKE_BUILD_TYPE=Release \
  -DCATFISH_ENABLE_SYZYGY=ON \
  -DCATFISH_FATHOM_ROOT=/absolute/path/to/Fathom
cmake --build build/syzygy
```

Then configure the UCI process:

```text
setoption name SyzygyPath value /absolute/path/to/syzygy/files
```

Catfish probes DTZ at the root, respects the halfmove clock, rejects positions
with castling rights, validates Fathom's result against its own legal moves,
and falls back to search on any probe miss.

Fallback build:

```bash
make
make test
```

## Test and quality commands

```bash
npm run lint
npm run typecheck
npm test
npm run test:e2e
npm run build
```

Engine interoperability (UCI process tests and Fastchess helpers):

```bash
npm run test:uci-process
npm run test:uci-compliance   # requires Fastchess on PATH
# STOCKFISH=/path/to/stockfish npm run test:engine-smoke
```

See [tools/engine-testing/README.md](tools/engine-testing/README.md) and
[docs/ENGINE_TESTING.md](docs/ENGINE_TESTING.md).

The Playwright suite runs the same gameplay flows at desktop and Pixel 7
viewports. Install its Chromium runtime once with:

```bash
npx playwright install chromium
```

## Configuration

Copy `.env.example` to `.env` for local overrides:

| Variable | Default | Purpose |
| --- | --- | --- |
| `CATFISH_ENGINE_PATH` | automatic build-path lookup | Trusted path to `catfish_uci` |
| `CATFISH_HOST` | `127.0.0.1` | Bridge bind address |
| `CATFISH_PORT` / `PORT` | `8787` | Bridge port; hosts generally inject `PORT` |
| `CATFISH_HASH_MB` | `32` | Native transposition-table size |
| `CATFISH_MAX_PENDING_SEARCHES` | `8` | Maximum active and queued searches |
| `CATFISH_RATE_LIMIT_MAX` | `60` | Search requests allowed per IP/window |
| `CATFISH_RATE_LIMIT_WINDOW_MS` | `60000` | In-process rate-limit window |
| `CATFISH_OWN_BOOK` | engine default | Enable or disable the built-in book |
| `CATFISH_BOOK_PATH` | unset | Trusted external opening-book path |
| `CATFISH_SYZYGY_PATH` | unset | Optional local tablebase path |

Without an override, the bridge checks `build/debug`, `build/release`, then
`build/make`. The executable path never comes from browser input.

## Architecture

```text
React + TypeScript + chess.js + react-chessboard
                         |
                      JSON/HTTP
                         |
           Express + typed UCI adapter
                         |
                    stdin/stdout
                         |
             catfish_uci -> catfish_core
```

`chess.js` provides immediate UI legality, SAN, PGN, and browser-side game
status. Catfish remains authoritative for engine search. The server validates
the submitted FEN and serializes requests to one persistent engine process.

See:

- [Architecture](ARCHITECTURE.md)
- [Architecture decision](docs/adr/0001-chess-ui-architecture.md)
- [UCI protocol](docs/UCI.md)
- [Engine testing](docs/ENGINE_TESTING.md)
- [Local API](docs/API.md)

## Security boundaries

- The service binds to loopback by default and has no authentication.
- Production defaults to `0.0.0.0` and honors platform-provided `PORT`.
- JSON bodies are limited to 8 KB.
- FEN is validated server-side.
- Search depth is restricted to 1–5 through the public API.
- The engine is started with `spawn`, an argument array, and no shell.
- Searches are serialized so protocol responses cannot cross requests.
- Search queue length and per-IP request rates are bounded.
- The production response sets clickjacking, MIME-sniffing, referrer, and
  browser-permission security headers.

The included rate and queue limits are appropriate safeguards for a small
public demo, not a distributed abuse-prevention system. For higher traffic,
add platform/CDN rate limiting, monitoring, and stronger isolation.

## Current limitations

- Search is fixed-depth at the API boundary and synchronous.
- UCI `stop` is safe while idle but cannot interrupt a running search.
- UCI time controls and `go movetime` are not implemented.
- The bridge sends a FEN per request, so pre-root repetition history remains
  browser-owned; Catfish detects repetitions formed inside its search and when
  UCI positions include their move list.
- Syzygy integration currently probes DTZ at the root; WDL probing inside the
  search tree is a future optimization.
- The built-in opening repertoire is intentionally compact and curated, not a
  replacement for a statistically generated large Polyglot book.
- Game state is intentionally device-memory-only and resets on reload.
- Deployment requires a long-lived Linux container service; static and
  request-scoped serverless hosts cannot retain the native UCI child process.

## Browser support

The production build targets Vite's current Baseline Widely Available browser
set. Current Chromium, Firefox, and Safari are the intended targets. Critical
piece assets ship with `react-chessboard`; the app does not depend on a
third-party asset CDN at runtime.

## Troubleshooting

### Engine executable not found

Run `npm run build:engine`, or set `CATFISH_ENGINE_PATH` to an absolute or
workspace-relative `catfish_uci` path.

### Port already in use

Stop the process using 5173 or 8787, or change `CATFISH_PORT` and update the
Vite proxy for development.

### UI says Engine offline

Check the bridge terminal for a UCI startup diagnostic. Rebuild the engine,
then use the in-app Retry action or restart `npm run dev`.

### CMake is unavailable

Use `make`, then point `CATFISH_ENGINE_PATH` at
`build/make/catfish_uci`.
