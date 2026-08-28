#pragma once

#include "catfish/types.hpp"

#include <array>
#include <string>

namespace catfish {

constexpr Bitboard bit(Square square) {
    return 1ULL << square;
}

constexpr bool has_bit(Bitboard board, Square square) {
    return (board & bit(square)) != 0;
}

int popcount(Bitboard board);
Square lsb(Bitboard board);
Square pop_lsb(Bitboard& board);
std::string bitboard_to_string(Bitboard board);

extern std::array<Bitboard, 64> kKnightAttacks;
extern std::array<Bitboard, 64> kKingAttacks;
extern std::array<std::array<Bitboard, 64>, 2> kPawnAttacks;

void init_bitboards();

} // namespace catfish
