#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include<vector>

struct Move{
    int from;
    int to;
    int promotion; // 0 for no promotion, otherwise the piece type to promote t
    int capture; // 0 for no capture, otherwise the piece type captured
    int enPassant; // 0 for no en passant, otherwise the square of the
};

void generatePawnMoves(const Position &pos, std::vector<Move> &moves);

void initKnightAttacks();
void generateKnightMoves(const Position &pos, Color side, std::vector<Move> &moves);

#endif 