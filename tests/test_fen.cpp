#include "catfish/fen.hpp"

#include "test_support.hpp"

namespace catfish_tests {

void test_fen_round_trip() {
    const auto board = catfish::board_from_fen(catfish::kStartFen);
    CATFISH_EXPECT_EQ(catfish::board_to_fen(board), std::string(catfish::kStartFen));
}

void test_fen_state() {
    const auto board = catfish::board_from_fen("8/8/8/3pP3/8/8/8/4K2k w - d6 0 42");
    CATFISH_EXPECT_EQ(static_cast<int>(board.side_to_move()), static_cast<int>(catfish::Color::White));
    CATFISH_EXPECT_EQ(board.en_passant_square(), catfish::make_square(3, 5));
    CATFISH_EXPECT_EQ(board.fullmove_number(), 42);
}

} // namespace catfish_tests
