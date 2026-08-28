# Catfish UCI protocol

Run `catfish_uci` as a persistent process and communicate with newline-delimited
text over stdin/stdout. Diagnostics are written to stderr.

## Supported commands

| Command | Behavior |
| --- | --- |
| `uci` | Emits engine identity and `uciok` |
| `isready` | Emits `readyok` |
| `ucinewgame` | Resets the session to the starting position |
| `setoption name Hash value N` | Resizes the transposition table to 1–1024 MB |
| `setoption name Clear Hash` | Clears cached search state |
| `setoption name OwnBook value true/false` | Enables or disables opening-book moves |
| `setoption name BookFile value PATH` | Transactionally loads an external text book |
| `setoption name SyzygyPath value PATH` | Initializes optional local Fathom tables |
| `position startpos [moves ...]` | Loads start position and verified legal moves |
| `position fen <six fields> [moves ...]` | Loads a FEN and verified legal moves |
| `go depth N` | Searches synchronously at depth 1–20 |
| `stop` | Safe no-op while idle; current search is not interruptible |
| `quit` | Exits cleanly |

Moves use long coordinate notation: `e2e4`, `e1g1`, and promotion strings such
as `a7a8q`. Every input move is matched against Catfish's generated legal
moves. A failed `position` command is transactional and preserves the prior
board.

## Output

Example:

```text
id name Catfish
id author Catfish contributors
uciok
readyok
option name Hash type spin default 32 min 1 max 1024
option name OwnBook type check default true
info depth 3 seldepth 6 score cp 18 nodes 1287 qnodes 814 tbhits 0 hashhits 42 pv e2e4 e7e5 g1f3
bestmove e2e4
```

A terminal position emits `bestmove 0000`. Scores are relative to the side to
move at the searched root, as expected by UCI. `qnodes` and `hashhits` are
Catfish extensions; unrecognized UCI tokens can be ignored. A book hit emits
`info string book <name>`. A tablebase hit emits `info string syzygy ...` and
increments the standard `tbhits` value.

## Not implemented

- `go movetime`
- Clock and increment fields (`wtime` / `btime` / `winc` / `binc` / `movestogo`)
- `go nodes` and `go infinite`
- Pondering
- Interruptible search (`stop` cannot cancel an in-flight search)
- Iterative `info` output
- WDL tablebase probes inside the search tree

Until time management lands, treat Catfish as fixed-depth only for fair
matches. Clock-style `go` commands currently fall back to depth 3.

Tournament runners, compliance checks, and opponent manifests live under
[tools/engine-testing](../tools/engine-testing/README.md). The full ladder
(Fastchess → timed UCI → Lichess via lichess-bot) is documented in
[ENGINE_TESTING.md](ENGINE_TESTING.md).
