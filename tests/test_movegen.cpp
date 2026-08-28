#include "catfish/fen.hpp"
#include "catfish/movegen.hpp"

#include "test_support.hpp"

#include <algorithm>

namespace catfish_tests {

namespace {

bool has_move(const std::vector<catfish::Move>& moves, const std::string& text) {
    return std::any_of(moves.begin(), moves.end(), [&](const catfish::Move& move) {
        return catfish::move_to_string(move) == text;
    });
}

} // namespace

void test_start_position_legal_moves() {
    auto board = catfish::Board::start_position();
    const auto moves = catfish::generate_legal_moves(board);
    CATFISH_EXPECT_EQ(moves.size(), static_cast<std::size_t>(20));
    CATFISH_EXPECT_TRUE(has_move(moves, "e2e4"));
    CATFISH_EXPECT_TRUE(has_move(moves, "g1f3"));
}

void test_castling_generation() {
    auto board = catfish::board_from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    const auto moves = catfish::generate_legal_moves(board);
    CATFISH_EXPECT_TRUE(has_move(moves, "e1g1"));
    CATFISH_EXPECT_TRUE(has_move(moves, "e1c1"));
}

void test_en_passant_generation() {
    auto board = catfish::board_from_fen("8/8/8/3pP3/8/8/8/4K2k w - d6 0 1");
    const auto moves = catfish::generate_legal_moves(board);
    CATFISH_EXPECT_TRUE(has_move(moves, "e5d6"));
}

void test_promotion_generation() {
    auto board = catfish::board_from_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    const auto moves = catfish::generate_legal_moves(board);
    CATFISH_EXPECT_TRUE(has_move(moves, "a7a8q"));
    CATFISH_EXPECT_TRUE(has_move(moves, "a7a8n"));
}

} // namespace catfish_tests

