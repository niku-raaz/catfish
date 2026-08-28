#include "catfish/board.hpp"
#include "catfish/fen.hpp"
#include "catfish/uci.hpp"
#include "catfish/zobrist.hpp"

#include "test_support.hpp"

namespace catfish_tests {

namespace {

void expect_move_preserves_incremental_hash(
    const char* fen,
    const char* move_text
) {
    auto board = catfish::board_from_fen(fen);
    const auto initial_key = board.position_key();
    const auto move = catfish::move_from_uci(board, move_text);
    CATFISH_EXPECT_TRUE(move.has_value());
    const auto undo = board.make_move(*move);
    CATFISH_EXPECT_EQ(board.position_key(), catfish::compute_zobrist(board));
    board.unmake_move(undo);
    CATFISH_EXPECT_EQ(board.position_key(), initial_key);
    CATFISH_EXPECT_EQ(board.position_key(), catfish::compute_zobrist(board));
}

} // namespace

void test_incremental_zobrist() {
    auto board = catfish::Board::start_position();
    const auto initial_key = board.position_key();
    CATFISH_EXPECT_EQ(initial_key, catfish::compute_zobrist(board));

    const auto move = catfish::move_from_uci(board, "e2e4");
    CATFISH_EXPECT_TRUE(move.has_value());
    const auto undo = board.make_move(*move);
    CATFISH_EXPECT_EQ(board.position_key(), catfish::compute_zobrist(board));

    board.unmake_move(undo);
    CATFISH_EXPECT_EQ(board.position_key(), initial_key);
    CATFISH_EXPECT_EQ(board.position_key(), catfish::compute_zobrist(board));

    expect_move_preserves_incremental_hash(
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "e1g1"
    );
    expect_move_preserves_incremental_hash(
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
        "a7a8q"
    );
    expect_move_preserves_incremental_hash(
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1",
        "e5d6"
    );

    const auto non_capturable_ep = catfish::board_from_fen(
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"
    );
    const auto canonical_ep = catfish::board_from_fen(
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1"
    );
    CATFISH_EXPECT_EQ(non_capturable_ep.position_key(), canonical_ep.position_key());
}

} // namespace catfish_tests
