#include "catfish/fen.hpp"
#include "catfish/opening_book.hpp"
#include "catfish/search.hpp"
#include "catfish/uci.hpp"

#include "test_support.hpp"

#include <vector>

namespace catfish_tests {

void test_builtin_opening_book() {
    catfish::OpeningBook book;
    auto board = catfish::Board::start_position();
    const auto choice = book.probe(board);
    CATFISH_EXPECT_TRUE(choice.has_value());
    CATFISH_EXPECT_EQ(catfish::move_to_string(choice->move), std::string("e2e4"));
    CATFISH_EXPECT_TRUE(book.position_count() > 20);

    auto after_e4 = catfish::board_from_fen(
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1"
    );
    const auto black_choice = book.probe(after_e4);
    CATFISH_EXPECT_TRUE(black_choice.has_value());
    CATFISH_EXPECT_EQ(catfish::move_to_string(black_choice->move), std::string("e7e5"));
}

void test_search_finds_mate_at_horizon() {
    auto board = catfish::board_from_fen("7k/8/5KQ1/8/8/8/8/8 w - - 0 1");
    const auto result = catfish::search_best_move(board, 1);
    CATFISH_EXPECT_TRUE(result.best_move.has_value());
    CATFISH_EXPECT_EQ(catfish::move_to_string(*result.best_move), std::string("g6g7"));
    CATFISH_EXPECT_TRUE(result.score > 99000);
    CATFISH_EXPECT_TRUE(!result.principal_variation.empty());
}

void test_search_repetition_and_transposition_table() {
    auto board = catfish::Board::start_position();
    std::vector<catfish::ZobristKey> history{board.position_key()};
    for (const char* move : {
             "g1f3", "g8f6", "f3g1", "f6g8",
             "g1f3", "g8f6", "f3g1", "f6g8"
         }) {
        CATFISH_EXPECT_TRUE(catfish::apply_uci_move(board, move));
        history.push_back(board.position_key());
    }
    catfish::SearchEngine engine(1);
    const auto draw = engine.search(board, 3, history);
    CATFISH_EXPECT_EQ(draw.score, 0);
    CATFISH_EXPECT_TRUE(!draw.best_move.has_value());

    auto start = catfish::Board::start_position();
    const auto first = engine.search(start, 3);
    const auto second = engine.search(start, 3);
    CATFISH_EXPECT_TRUE(first.best_move.has_value());
    CATFISH_EXPECT_TRUE(second.best_move.has_value());
    CATFISH_EXPECT_TRUE(second.transposition_hits > 0);
}

} // namespace catfish_tests
