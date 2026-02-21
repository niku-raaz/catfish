#ifndef BOARD_H
#define BOARD_H

#include<cstdint>

enum Color { 
    WHITE, BLACK 
};

enum PieceType {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};


struct Position {
    uint64_t whitePawns;
    uint64_t whiteKnights;
    uint64_t whiteBishops;
    uint64_t whiteRooks;
    uint64_t whiteQueens;
    uint64_t whiteKing;

    uint64_t blackPawns;
    uint64_t blackKnights;
    uint64_t blackBishops;
    uint64_t blackRooks;
    uint64_t blackQueens;
    uint64_t blackKing;

    bool whiteToMove;

    // Castling rights (whether the king/rook have not moved)
    bool whiteCanCastleK;  // kingside
    bool whiteCanCastleQ;  // queenside
    bool blackCanCastleK;
    bool blackCanCastleQ;

    // En passant: square a pawn can capture to (0-63), or -1 if none
    int epSquare;
};

struct Move {
    int from;
    int to;
    PieceType piece;
    int captured;  // -1 means no capture
    bool isCastling;
    bool isEnPassant;
    int promotion;  // -1 = none, else KNIGHT/BISHOP/ROOK/QUEEN
};

struct State {
    Position position;
};

// starting position
void setStartPosition(Position &pos);

//telling positions
uint64_t whitePieces(const Position &pos);
uint64_t blackPieces(const Position &pos);
uint64_t allPieces(const Position &pos);

// printing 
void printBoard(const Position &pos);

// make and unmake moves
void makeMove(Position &pos, const Move &move);
void undoMove(Position &pos, const State &previous);

#endif
