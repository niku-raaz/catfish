# Catfish Chess Engine

Catfish is a C++ bitboard-based chess engine developed with a focus on a structured, verifiable implementation of fundamental chess programming concepts. The engine follows a multi-phase development workflow to ensure the correctness of move generation and search algorithms before adding complexity.

## Project Structure

The project is organized into a core engine directory containing the bitboard logic, move generation, and search implementations:

* **`engine/board.h` / `board.cpp**`: Defines the `Position` and `Move` structures. Handles bitboard manipulation, piece tracking for both colors, castling rights, and en passant state.
* **`engine/movegen.h` / `movegen.cpp**`: Implements move generation for all pieces, including special moves like castling, en passant, and promotions. It also includes check detection and square attack logic.
* **`engine/search.h` / `search.cpp**`: Contains the `perft` testing function and the `minimax` search algorithm.
* **`engine/eval.h` / `eval.cpp**`: Static evaluation function that scores positions based on material.
* **`engine/main.cpp`**: Entry point for verifying the engine's current phase through manual tests and evaluation checks.

## Technical Features

### Board Representation

Catfish uses **bitboards** (64-bit integers) to represent piece locations. The `Position` struct tracks bitboards for each piece type and color, along with crucial state information:

* Side to move.
* Castling rights for both King and Queen side.
* En passant square.

### Move Generation

The engine implements a legal move filter during search and testing. Moves are generated for all piece types and then validated by checking if the move leaves the king in check.

* **Piece Movements**: Pawns, Knights, Bishops, Rooks, Queens, and Kings.
* **Special Moves**: Castling, En Passant, and Pawn Promotion (to Knight, Bishop, Rook, or Queen).

### Search and Evaluation

* **Minimax Search**: A fixed-depth search that returns scores from White's perspective (positive for White, negative for Black).
* **Static Evaluation**: Currently uses material-only values:
* Pawn: 100
* Knight: 320
* Bishop: 330
* Rook: 500
* Queen: 900


* **Perft Testing**: Used to verify move generation accuracy against standard reference nodes.

## Development Status

According to the internal workflow, the engine has implemented and verified the following:

| Component | Status |
| --- | --- |
| **Board Logic** | Bitboards, make/undo, and state tracking (castling, ep) are done. |
| **Move Generation** | All pieces and special moves are implemented. |
| **Check Detection** | `isSquareAttacked` and `isKingInCheck` are functional. |
| **Perft** | Legal filter applied; verified against standard reference values. |
| **Static Eval** | Material-based scoring implemented. |
| **Minimax** | Fixed-depth search (depths 0, 1, and 2) verified. |

## Future Roadmap

The engine is designed to progress through the following subsequent phases:

1. **Alpha-Beta Pruning**: Implementing efficient pruning to reduce the number of nodes searched.
2. **Best Move at Root**: Returning the specific best move found at a given depth.
3. **Iterative Deepening**: Adding time management and progressive depth searching.
4. **UCI Protocol**: Enabling compatibility with external Chess GUIs.

## Getting Started

### Building the Engine

The engine can be compiled using a C++ compiler:

```bash
g++ engine/*.cpp -o catfish

```

### Running Tests

The `main.cpp` file contains built-in verification tests for evaluation and minimax. Running the compiled executable will output evaluation scores for the starting position and several test cases (e.g., adding a Queen or Rook) to ensure the engine is calculating correctly.
