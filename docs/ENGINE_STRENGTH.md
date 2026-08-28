# Engine strength architecture

Catfish improves strength in layers so correctness can be measured separately
from speed. The browser never selects a computer move.

## Search pipeline

```text
root position
  -> Syzygy DTZ probe, when configured and applicable
  -> legal built-in/external opening-book probe
  -> iterative deepening
       -> transposition probe and TT move
       -> principal-variation alpha-beta
       -> quiescence at the horizon
       -> repetition, fifty-move, insufficient-material, mate/stalemate checks
```

The table is intentionally persistent across UCI searches. `ucinewgame` and
`Clear Hash` remove stale search state. Mate scores are adjusted by ply when
stored, ensuring that transpositions do not change mate distance.

## Evaluation

Evaluation is always returned from White's perspective. Search converts it to
the side-to-move perspective and adds a small tempo bonus.

The phase value ranges from 24 in material-rich positions to 0 in pawn
endgames. Each term has independent middlegame and endgame weights and is
interpolated by phase. Run:

```bash
./build/debug/catfish evaltrace "<fen>"
```

to inspect the components.

## Opening data

The embedded book is deliberately small, deterministic, and auditable. It
covers representative open, semi-open, closed, flank, and Indian structures.
Lines that share a position accumulate their weights. The highest-weight legal
candidate is selected.

For a larger repertoire, create a tab-separated file:

```text
weight<TAB>name<TAB>uci move sequence
```

Generate large books offline from data with a compatible license. Runtime
opening-explorer network calls are intentionally excluded.

## Syzygy boundary

Fathom is optional and is compiled only when explicitly configured. Root DTZ
probing is deterministic, local, bounded by the installed table cardinality,
and guarded by Catfish legal-move validation. A failed probe is indistinguishable
from a normal miss to search. Tablebase files are not bundled.

## Measurement

`catfish bench [depth]` searches a fixed five-position suite and reports total
nodes, quiescence nodes, hash hits, elapsed milliseconds, and NPS. It is a
performance regression tool, not proof of playing strength.

Before accepting selective pruning or evaluation retuning:

1. Run perft and all unit tests.
2. Compare release `bench` results.
3. Run fixed tactical and endgame regression positions.
4. Play paired self-play games from the same openings with colors reversed
   (Fastchess under `tools/engine-testing/`; OwnBook off; never treat equal
   depths as equal strength).
5. Prefer candidate-vs-baseline and weakened-Stockfish ladders over
   full-strength Stockfish win rate; retain a change only after the result is
   statistically credible (see [ENGINE_TESTING.md](ENGINE_TESTING.md)).

Useful primary references:

- [Stockfish search implementation](https://github.com/official-stockfish/Stockfish/blob/master/src/search.cpp)
- [Stockfish transposition table](https://github.com/official-stockfish/Stockfish/blob/master/src/tt.cpp)
- [Stockfish NNUE documentation](https://github.com/official-stockfish/nnue-pytorch/blob/master/docs/nnue.md)
- [Fathom Syzygy probing](https://github.com/jdart1/Fathom)
- [Lichess CC0 opening names](https://github.com/lichess-org/chess-openings)
- [Lichess open game database](https://database.lichess.org/)
