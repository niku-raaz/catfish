# Architecture

Catfish is organized around a small core engine library, `catfish_core`, with CLI and test executables built on top.

The local application adds protocol, server, and browser layers without adding
web dependencies to the engine core:

```text
web/ (React + chess.js)
        |
server/ (Express + validation + UCI lifecycle)
        |
catfish_uci (standard input/output protocol)
        |
catfish_core (C++ chess logic and search)
```

## Modules

### Types and Bitboards

- `include/catfish/types.hpp`
- `include/catfish/bitboard.hpp`

Core aliases and enums live here. Squares use `0..63`, where `a1 = 0` and `h8 = 63`. Piece locations are represented with `uint64_t` bitboards.

### Board

- `include/catfish/board.hpp`
- `src/board.cpp`

`Board` owns the complete chess state:

- `pieces[2][6]` bitboards.
- Side to move.
- Castling rights.
- En passant square.
- Halfmove and fullmove counters.
- An incrementally maintained deterministic Zobrist position key.

Move application returns an `UndoState`, allowing search and perft to make and unmake moves without copying the full board at every node.

### FEN

- `include/catfish/fen.hpp`
- `src/fen.cpp`

FEN is the primary interchange format for tests, debugging, and future UCI support.

### Move Generation

- `include/catfish/movegen.hpp`
- `src/movegen.cpp`

Move generation is split into:

- Pseudo-legal move generation.
- Legal filtering by making a move and rejecting moves that leave the moving king in check.

Sliding pieces currently use simple ray scanning. This keeps correctness easy to audit before introducing magic bitboards or other lookup-heavy optimizations.

### Perft

- `include/catfish/perft.hpp`
- `src/perft.cpp`

Perft validates legal move generation. `perft_divide` prints move-by-move node counts for debugging.

### Evaluation

- `include/catfish/eval.hpp`
- `src/eval.cpp`

Evaluation returns a score from White's perspective. Positive is better for
White and negative is better for Black. Middlegame and endgame values are
tapered using remaining material. Explainable terms include material,
piece-square placement, mobility, passed/connected/doubled/isolated pawns,
king shelter and pressure, bishop pair, and rook files. `evaltrace` exposes the
component breakdown.

### Search

- `include/catfish/search.hpp`
- `src/search.cpp`

`SearchEngine` uses iterative deepening, principal-variation search,
quiescence, mate-distance scores, repetition/fifty-move/material draw checks,
TT move ordering, killer moves, and the history heuristic. Its persistent,
configurable transposition table stores bounds, depth, static evaluation, best
move, and generation. Results contain the completed depth, selective depth,
full principal variation, node classes, and hash hits.

### Hashing

- `include/catfish/zobrist.hpp`
- `src/zobrist.cpp`

Zobrist hashing is deterministic and updated in `Board::make_move`; the
previous key is part of `UndoState`. Tests compare incremental keys with a
complete recomputation after moves and unmake.

### Opening book and tablebases

- `include/catfish/opening_book.hpp`
- `src/opening_book.cpp`
- `include/catfish/tablebase.hpp`
- `src/tablebase.cpp`

The engine—not React or Node—selects opening and tablebase moves. The compact
built-in book and transactional external text books are keyed by Catfish
position keys, and every candidate is checked against generated legal moves.
The optional Fathom adapter performs local Syzygy root DTZ probing without
introducing networking into `catfish_core`.

### UCI

- `include/catfish/uci.hpp`
- `src/uci.cpp`
- `src/uci_main.cpp`

`catfish_uci` owns protocol state, translates coordinate strings by matching
generated legal moves, applies `position` commands transactionally, and emits
fixed-depth `info` plus `bestmove` output. Protocol diagnostics use stderr.
The session retains a search engine, hash table, book, optional tablebase, and
position-key history across commands.

### Local engine bridge

- `server/uciEngine.ts`
- `server/app.ts`
- `server/contracts.ts`

The bridge owns one persistent native child process and performs the UCI
handshake. It reconstructs the engine position from a server-validated FEN for
each search. Line buffering handles arbitrary stdout chunks, waiters have
timeouts, requests are serialized, and process exits reject pending work.

The public boundary is intentionally small:

- `GET /api/health`
- `POST /api/search`

### Web application

- `web/hooks/useChessGame.ts`
- `web/components/`
- `web/chess/`

React owns presentation state. A single `Chess` instance from `chess.js`
maintains legal browser interaction, SAN, PGN, history-dependent draw rules,
and synchronization. Each Catfish move is applied through `chess.js`; an
illegal response becomes a visible error rather than corrupting the game.

## Executables

- `catfish`: general CLI for board printing, eval, perft, and search.
- `catfish_uci`: standard UCI executable used by the local bridge and chess GUIs.
- `catfish_perft`: perft divide runner.
- `catfish_tests`: lightweight test executable registered with CTest.

## Concurrency and ownership

The C++ search is synchronous and not treated as thread-safe. The Node adapter
serializes all work for its engine process. The browser also locks board input
while a request is pending. Reset, undo, and new-game operations invalidate
stale request identifiers before their response can update UI state.

## Deployment boundary

The production Docker image compiles an optimized native Catfish executable,
the Node bridge, and browser assets in a multi-stage build. Its non-root
runtime serves the UI and API from one origin, honors `0.0.0.0:$PORT`, exposes
an engine-aware health check, bounds the search queue, and shuts the engine
down on `SIGTERM`. It can run on any long-lived Linux container service.

The application is not compatible with an edge-only/static runtime because it
requires a persistent native child process. WebAssembly in a Web Worker remains
the migration path for a browser-only release.
