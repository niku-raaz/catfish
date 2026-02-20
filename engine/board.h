#ifndef BOARD_H
#define BOARD_H

#include<cstdint>

enum Color { WHITE, BLACK };

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
};

// starting position
void setStartPosition(Position &pos);

//telling positions
uint64_t whitePieces(const Position &pos);
uint64_t blackPieces(const Position &pos);
uint64_t allPieces(const Position &pos);

// printing 
void printBoard(const Position &pos);

#endif
