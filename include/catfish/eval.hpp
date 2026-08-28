#pragma once

#include "catfish/board.hpp"

namespace catfish {

constexpr int kPawnValue = 100;
constexpr int kKnightValue = 320;
constexpr int kBishopValue = 330;
constexpr int kRookValue = 500;
constexpr int kQueenValue = 900;
constexpr int kKingValue = 20000;

struct EvaluationBreakdown {
    int material{0};
    int piece_square{0};
    int pawn_structure{0};
    int mobility{0};
    int king_safety{0};
    int positional{0};
    int phase{0};
    int score{0};
};

int evaluate(const Board& board);
EvaluationBreakdown evaluate_with_breakdown(const Board& board);

} // namespace catfish
