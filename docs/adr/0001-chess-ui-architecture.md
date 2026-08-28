# ADR 0001: Chess UI architecture

- Status: accepted
- Date: 2026-07-29

## Context

Catfish is a C++20 library and CLI with legal move generation and synchronous,
fixed-depth alpha-beta search. It has no network boundary, standard engine
protocol, time management, cancellation, repetition history, or browser UI.
The new application must preserve the engine core and let a browser play
against Catfish without treating another engine as authoritative.

## Discovery summary

- Moves are serialized as long algebraic/UCI coordinates (`e2e4`, `a7a8q`).
- Move parsing did not exist; legal moves were available for matching.
- Search makes and unmakes moves and leaves its root board unchanged.
- No legal move plus check represents checkmate; no legal move without check
  represents stalemate.
- Search is synchronous, fixed-depth, and cannot be interrupted.
- Fifty-move counters exist in FEN, but automatic fifty-move, repetition, and
  insufficient-material adjudication did not exist in the engine.
- The engine is not documented as thread-safe. Each game therefore owns one
  process and serializes work.

## Decision

Use a local three-layer application:

```text
React 19 + TypeScript + Vite
            |
        JSON/HTTP
            |
Express + typed UCI process adapter
            |
       stdin/stdout
            |
catfish_uci -> catfish_core
```

The UI uses `chess.js` for immediate browser interaction, SAN, PGN, and
game-status presentation. The server validates inputs and obtains every
computer move from Catfish. UCI is the stable engine boundary.

The first release uses request/response HTTP. Catfish emits a final `info` line
and `bestmove`; streaming transport would not add visible value until the
search supports iterative updates.

## Options considered

| Option | Benefits | Costs | Decision |
| --- | --- | --- | --- |
| UCI + Node process bridge | Standard GUI compatibility, isolates native engine, straightforward tests | Local native service required; process lifecycle to manage | Selected |
| Custom JSON engine protocol | Simple app-specific messages | Duplicates a standard, couples engine to this UI | Rejected |
| Emscripten + Web Worker | Static hosting, no backend, local computation | New toolchain/bindings, larger first change, search cancellation and worker packaging work | Future path |
| C++ HTTP service | One native service and fewer languages | More networking/security code in C++; slower UI iteration | Rejected |

## Package selection

Versions were checked against the npm registry on 2026-07-29.

| Package | Version reviewed | License | Selection rationale |
| --- | ---: | --- | --- |
| React | 19.2.8 | MIT | Mature component model and first-class TypeScript guidance |
| TypeScript | 6.0.3 | Apache-2.0 | Newest release supported by the current typed lint stack |
| Vite | 8.1.5 | MIT | Fast typed React template and optimized static build |
| react-chessboard | 5.10.0 | MIT | React-native, responsive, typed, drag/click, mobile and accessibility support |
| Chessground | 9.2.1 | GPL-3.0+ | Excellent and small, but GPL changes distribution obligations; not selected |
| chess.js | 1.4.0 | BSD-2-Clause | Typed legal moves, FEN, SAN, PGN and terminal-state detection without AI |
| Express | 5.2.1 | MIT | Small, conventional local HTTP boundary |
| Zod | 4.4.3 | MIT | Strict runtime validation at the browser/server boundary |
| Vitest | 4.1.10 | MIT | Vite-aligned TypeScript tests |
| Playwright | 1.62.0 | Apache-2.0 | Cross-browser end-to-end and mobile viewport tests |

Primary references:

- [React TypeScript guide](https://react.dev/learn/typescript)
- [Vite guide](https://vite.dev/guide/)
- [react-chessboard documentation](https://react-chessboard.vercel.app/)
- [Chessground repository and license](https://github.com/lichess-org/chessground)
- [chess.js documentation](https://jhlywa.github.io/chess.js/)
- [Node child process API](https://nodejs.org/api/child_process.html)
- [Emscripten C++/JavaScript integration](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/)

## Responsibility boundaries

- `catfish_core`: board, legal moves, evaluation, search.
- `catfish_uci`: standard protocol and legal coordinate-move application.
- Server: process lifecycle, serialized searches, validation, timeouts.
- `chess.js`: responsive client legality, SAN/PGN, UI game adjudication.
- React: presentation and interaction only.

The browser sends a complete FEN for each search. The server reconstructs the
engine position from that FEN, and the browser rejects an illegal engine reply.
This makes synchronization failures visible instead of compounding them.

## Security

- The engine path comes only from trusted configuration or fixed build paths.
- The server uses `spawn` without a shell.
- Search depth, FEN size, JSON body size, and request rate/concurrency are
  bounded.
- Only one outstanding search is allowed per engine adapter.
- The default server binds to loopback and is intended for local play.

## Tradeoffs and limitations

This design requires a native process and cannot be deployed unchanged to an
edge/static host. Search is currently synchronous, so `stop` is safe while
idle but cannot interrupt a running search. The first UI exposes fixed depth,
not clocks. Browser draw adjudication is more complete than the C++ engine,
whose search does not yet model repetition history.

## Future migration

The UCI adapter remains useful for desktop chess GUIs. A later deployment can
compile `catfish_core` with Emscripten and place the same request contract
behind a Web Worker. Iterative deepening and an atomic stop flag can later
enable time controls, cancellation, and streamed analysis without changing the
React game model.
