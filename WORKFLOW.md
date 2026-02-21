# Chess Engine Development Workflow

This document defines the **task order** and **correctness checks** for the Catfish engine. Follow phases in order; verify each phase before moving to the next.

---

## Current State (as of this workflow)

| Component | Status | Notes |
|-----------|--------|--------|
| Board (bitboards, make/undo) | ✅ Done | Position, Move, State; castling rights, epSquare |
| Move generation | ✅ Done | All pieces, castling, en passant, promotion (Q/R/B/N) |
| Check detection | ✅ Done | `isSquareAttacked`, `isKingInCheck` |
| Perft | ✅ Legal filter applied | Counts only legal moves (make move → if king in check for mover, skip). Perft(1)=20, Perft(2)=400; deeper depths should match reference (see Phase 0). |
| Static eval | ✅ Done | `eval(pos)` in eval.h/eval.cpp; material only (P=100,N=320,B=330,R=500,Q=900); positive = White. Start=0, imbalances correct. |
| Minimax | ✅ Done | `minimax(pos, depth)` in search; depth 0 = eval; legal moves only; score for White, max (white) / min (black). Depth 0/1/2 verified. |

---

## Phase 0: Lock in legal move counting (do this first)

**Goal:** Perft counts **legal** moves only, so all later search is built on a correct baseline.

### Tasks

1. **Filter moves in perft**
   - After `makeMove(pos, move)`, check: if the side that **just moved** is still in check, the move is illegal.
   - Use: `Color movedSide = pos.whiteToMove ? BLACK : WHITE;` (turn already switched) then `if (isKingInCheck(pos, movedSide)) { undoMove(...); continue; }`.
   - Only add to node count when the move is legal (i.e. don’t count and don’t recurse when illegal).

2. **Correctness**
   - From **starting position**, run perft and compare to known values:
     - Perft(1) = **20**
     - Perft(2) = **400**
     - Perft(3) = **8,902**
     - Perft(4) = **197,281**
     - Perft(5) = **4,865,609**
   - If any depth disagrees, fix movegen or make/undo before continuing.

**Exit condition:** Perft(1)–Perft(5) from start position match the table above.

**Note:** If Perft(1) and Perft(2) match but deeper depths are off by a small amount, re-check castling (squares crossed), en passant (only on last half-move), and promotion (no duplicate/non-legal). Re-run perft after any movegen/board change.

---

## Phase 1: Static evaluation

**Goal:** One function that scores a position (no search). Positive = better for White.

### Tasks

1. **Add evaluation module**
   - New (or existing) file: e.g. `eval.h` / `eval.cpp`.
   - Declare: `int eval(const Position &pos);`

2. **Implement material + optional PST**
   - Piece values (e.g. pawn=100, knight=320, bishop=330, rook=500, queen=900, king=0 or huge).
   - Score = (white material + white PST) − (black material + black PST).
   - Optional: simple piece-square tables (e.g. encourage center, passed pawns).

3. **Correctness**
   - Start position: score ≈ 0 (or small value if PST is asymmetric).
   - Position with extra white queen: clearly positive.
   - Position with extra black rook: clearly negative.
   - No search yet; only call `eval(pos)` and print/assert.

**Exit condition:** `eval(startPosition)` near 0; obvious material imbalances have the correct sign and rough magnitude.

---

## Phase 2: Minimax (no pruning)

**Goal:** Fixed-depth search that returns the same value as “minimax with static eval at leaves.”

### Tasks

1. **Minimax in search**
   - Add e.g. `int minimax(Position &pos, int depth)`.
   - **Base case:** if `depth == 0`, return `eval(pos)`.
   - **Recursion:** generate **legal** moves (same as perft: make move → if king in check for mover, undo and skip). For each legal move: make, `score = -minimax(pos, depth - 1)`, undo; take **max** of scores (because we assume we’re maximizing from current side’s view; if your convention is “score for white”, then at each node you maximize or minimize depending on side to move).
   - Convention: keep **eval** and **minimax** in “white’s view” (positive = good for white). So at a node: if white to move, max over -minimax; if black to move, min over -minimax, or equivalently max over minimax with negated eval. Standard: `minimax` returns score for **side to move** (so negate in recursion), and at root you take max over moves.

2. **Clarify convention**
   - Recommended: `eval(pos)` = score for White. In minimax, score for side to move = max over moves of ( - minimax(opponent)). So root (white to move): best = max over moves of ( - minimax after move).

3. **Correctness**
   - Depth 0: minimax = eval (no moves).
   - Depth 1: generate legal moves, make each, eval resulting position, undo; root score = max (white) or min (black) of those evals. Compare with a couple of manual positions.

**Exit condition:** Depth-0 and depth-1 results match hand-calculated evals/minimax from a few test positions.

---

## Phase 3: Alpha-beta pruning

**Goal:** Same result as minimax, fewer nodes.

### Tasks

1. **Replace minimax with alpha-beta**
   - Signature e.g. `int alphaBeta(Position &pos, int depth, int alpha, int beta)`.
   - Base case: depth 0 → return eval(pos).
   - Loop over legal moves: make move, skip if king in check for mover; else `score = -alphaBeta(pos, depth-1, -beta, -alpha)`; undo; if `score >= beta` return beta (fail high); if `score > alpha` set `alpha = score`. Return alpha.

2. **Root call**
   - Call with alpha = -∞, beta = +∞ (e.g. -100000, 100000).

3. **Correctness**
   - On the same positions and depth, **score** must equal minimax score.
   - Optionally add a debug mode: count nodes; alpha-beta count should be ≤ minimax count.

**Exit condition:** Alpha-beta score matches minimax on several positions at depth 2–3; no illegal moves played.

---

## Phase 4: Best move at root

**Goal:** Return the move that achieves the best score at a given depth.

### Tasks

1. **Root search**
   - At root (depth = N), generate legal moves; for each move: make, score = -alphaBeta(depth N-1), undo. Track best score and best move. Return best move (and optionally score).

2. **Correctness**
   - From start position at depth 1, best move should be one of the 20 legal moves with the best eval. Manually check that the chosen move’s eval is max among the 20.
   - At depth 2+, sanity check: engine doesn’t hang, doesn’t play illegal moves, and score is consistent with minimax/alpha-beta.

**Exit condition:** Root returns a legal move and its score matches the alpha-beta score for that move.

---

## Phase 5: Iterative deepening (optional)

**Goal:** Search depth 1, then 2, then 3, … until time runs out; return best move from last completed depth.

### Tasks

1. **Loop depth**
   - For d = 1 to maxDepth (or until time limit): run root alpha-beta at depth d; update best move and score.
   - Optionally add a simple time limit (e.g. stop after 1 second).

2. **Correctness**
   - With no time limit, result should match a single full-depth search at that depth.

**Exit condition:** With no time limit, iterative deepening best move and score match single-depth search.

---

## Phase 6: UCI protocol (optional)

**Goal:** Engine can be used from GUIs (Arena, Scid, etc.).

### Tasks

1. **Parse UCI commands**
   - `uci`, `isready`, `ucinewgame`, `position startpos [moves ...]`, `go depth N` / `go movetime M`, `stop`, `quit`.
   - Maintain current position and play best move when asked.

2. **Correctness**
   - GUI connects; `position startpos` then `go depth 4` returns a legal move in UCI format (e.g. `e2e4`).

**Exit condition:** Engine plays legal moves from a GUI at fixed depth.

---

## Verification checklist (use anytime)

- [ ] **Perft** (from start): 20, 400, 8902, 197281, 4865609 at depth 1–5.
- [ ] **Eval**: Start ≈ 0; material imbalance has correct sign.
- [ ] **Minimax**: Depth 0 = eval; depth 1 = max/min of leaf evals.
- [ ] **Alpha-beta**: Same score as minimax at same depth.
- [ ] **Best move**: Legal; score matches alpha-beta for that move.
- [ ] **No illegal moves**: Every move in search leaves the moving side’s king not in check after makeMove.

---

## Task order summary

| Order | Phase | Main deliverable | Check |
|-------|--------|-------------------|--------|
| 0 | Legal perft | Perft counts legal moves only | Perft(1..5) = 20, 400, 8902, 197281, 4865609 |
| 1 | Static eval | `eval(pos)` | Start ≈ 0; imbalances correct |
| 2 | Minimax | `minimax(pos, depth)` | Depth 0/1 match hand calc |
| 3 | Alpha-beta | `alphaBeta(pos, depth, α, β)` | Same score as minimax |
| 4 | Best move | Root returns best move + score | Legal; score consistent |
| 5 | Iterative deepening | Depth loop + optional time | Same as single-depth |
| 6 | UCI | Parse go/position, output best move | GUI plays legal moves |

Stick to this order; run the correctness checks after each phase before moving on.
