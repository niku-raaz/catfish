#include "catfish/bitboard.hpp"

#include "test_support.hpp"

namespace catfish_tests {

void test_bitboard_helpers() {
    catfish::Bitboard board = catfish::bit(0) | catfish::bit(7) | catfish::bit(63);
    CATFISH_EXPECT_EQ(catfish::popcount(board), 3);
    CATFISH_EXPECT_EQ(catfish::pop_lsb(board), 0);
    CATFISH_EXPECT_EQ(catfish::pop_lsb(board), 7);
    CATFISH_EXPECT_EQ(catfish::pop_lsb(board), 63);
    CATFISH_EXPECT_EQ(board, 0ULL);
}

void test_attack_tables() {
    catfish::init_bitboards();
    CATFISH_EXPECT_EQ(catfish::popcount(catfish::kKnightAttacks[catfish::make_square(3, 3)]), 8);
    CATFISH_EXPECT_EQ(catfish::popcount(catfish::kKingAttacks[catfish::make_square(0, 0)]), 3);
}

} // namespace catfish_tests

