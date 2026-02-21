#include "eval.h"

// Piece values in centipawns (standard rough values)
static const int PIECE_VALUE[] = {
    100,   // PAWN
    320,   // KNIGHT
    330,   // BISHOP
    500,   // ROOK
    900,   // QUEEN
    0     // KING (not used in material; same for both sides)
};

static int countMaterial(const Position &pos, bool white) {
    int score = 0;
    if (white) {
        score += __builtin_popcountll(pos.whitePawns)   * PIECE_VALUE[PAWN];
        score += __builtin_popcountll(pos.whiteKnights) * PIECE_VALUE[KNIGHT];
        score += __builtin_popcountll(pos.whiteBishops) * PIECE_VALUE[BISHOP];
        score += __builtin_popcountll(pos.whiteRooks)   * PIECE_VALUE[ROOK];
        score += __builtin_popcountll(pos.whiteQueens)  * PIECE_VALUE[QUEEN];
    } else {
        score += __builtin_popcountll(pos.blackPawns)   * PIECE_VALUE[PAWN];
        score += __builtin_popcountll(pos.blackKnights) * PIECE_VALUE[KNIGHT];
        score += __builtin_popcountll(pos.blackBishops) * PIECE_VALUE[BISHOP];
        score += __builtin_popcountll(pos.blackRooks)   * PIECE_VALUE[ROOK];
        score += __builtin_popcountll(pos.blackQueens) * PIECE_VALUE[QUEEN];
    }
    return score;
}

int eval(const Position &pos) {
    return countMaterial(pos, true) - countMaterial(pos, false);
}
