#include "catfish/eval.hpp"
#include "catfish/fen.hpp"
#include "catfish/perft.hpp"
#include "catfish/search.hpp"

#include "test_support.hpp"

namespace catfish_tests {

void test_bitboard_helpers();
void test_attack_tables();
void test_tapered_evaluation();
void test_fen_round_trip();
void test_fen_state();
void test_start_position_legal_moves();
void test_castling_generation();
void test_en_passant_generation();
void test_promotion_generation();
void test_uci_move_parsing();
void test_uci_handshake_and_position();
void test_uci_invalid_position_is_transactional();
void test_uci_search_and_terminal_position();
void test_search_finds_mate_at_horizon();
void test_search_repetition_and_transposition_table();
void test_builtin_opening_book();
void test_incremental_zobrist();

void test_start_position_perft() {
    auto board = catfish::Board::start_position();
    CATFISH_EXPECT_EQ(catfish::perft(board, 1), 20ULL);
    CATFISH_EXPECT_EQ(catfish::perft(board, 2), 400ULL);
    CATFISH_EXPECT_EQ(catfish::perft(board, 3), 8902ULL);
}

void test_eval_and_search() {
    auto board = catfish::Board::start_position();
    CATFISH_EXPECT_EQ(catfish::evaluate(board), 0);

    auto result = catfish::search_best_move(board, 1);
    CATFISH_EXPECT_TRUE(result.best_move.has_value());
}

} // namespace catfish_tests

int main() {
    catfish_tests::test_bitboard_helpers();
    catfish_tests::test_attack_tables();
    catfish_tests::test_tapered_evaluation();
    catfish_tests::test_fen_round_trip();
    catfish_tests::test_fen_state();
    catfish_tests::test_start_position_legal_moves();
    catfish_tests::test_castling_generation();
    catfish_tests::test_en_passant_generation();
    catfish_tests::test_promotion_generation();
    catfish_tests::test_start_position_perft();
    catfish_tests::test_eval_and_search();
    catfish_tests::test_builtin_opening_book();
    catfish_tests::test_search_finds_mate_at_horizon();
    catfish_tests::test_search_repetition_and_transposition_table();
    catfish_tests::test_incremental_zobrist();
    catfish_tests::test_uci_move_parsing();
    catfish_tests::test_uci_handshake_and_position();
    catfish_tests::test_uci_invalid_position_is_transactional();
    catfish_tests::test_uci_search_and_terminal_position();
    return 0;
}
