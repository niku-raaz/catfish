#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include<vector>

void generatePawnMoves(const Position &pos, Color side, std::vector<Move> &moves);

void initKnightAttacks();
void generateKnightMoves(const Position &pos, Color side, std::vector<Move> &moves);

extern uint64_t kingAttacks[64];
void initKingAttacks();
void generateKingMoves(const Position &pos, Color side, std::vector<Move> &moves);
void generateCastlingMoves(const Position &pos, Color side, std::vector<Move> &moves);

void generateRookMoves(const Position &pos, Color side, std::vector<Move> &moves);
void generateBishopMoves(const Position &pos, Color side, std::vector<Move> &moves);
void generateQueenMoves(const Position &pos, Color side, std::vector<Move> &moves);

// Check detection (legality filtering)
bool isSquareAttacked(const Position &pos, int square, Color bySide);
bool isKingInCheck(const Position &pos, Color side);

#endif 