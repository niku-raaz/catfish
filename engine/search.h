#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "movegen.h"

uint64_t perft(Position &pos, int depth);

// Minimax: returns score for White (positive = good for White). Fixed depth, legal moves only.
int minimax(Position &pos, int depth);

#endif