#include "catfish/fen.hpp"
#include "catfish/uci.hpp"

#include "test_support.hpp"

#include <sstream>

namespace catfish_tests {

void test_uci_move_parsing() {
    auto board = catfish::Board::start_position();
    CATFISH_EXPECT_TRUE(catfish::move_from_uci(board, "e2e4").has_value());
    CATFISH_EXPECT_TRUE(!catfish::move_from_uci(board, "e2e5").has_value());
    CATFISH_EXPECT_TRUE(catfish::apply_uci_move(board, "e2e4"));
    CATFISH_EXPECT_TRUE(catfish::apply_uci_move(board, "e7e5"));

    auto promotion = catfish::board_from_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    CATFISH_EXPECT_TRUE(catfish::move_from_uci(promotion, "a7a8n").has_value());
}

void test_uci_handshake_and_position() {
    catfish::UciSession session;
    std::ostringstream output;
    std::ostringstream diagnostics;

    CATFISH_EXPECT_TRUE(session.handle_command("uci", output, diagnostics));
    CATFISH_EXPECT_TRUE(output.str().find("id name Catfish") != std::string::npos);
    CATFISH_EXPECT_TRUE(output.str().find("option name Hash") != std::string::npos);
    CATFISH_EXPECT_TRUE(output.str().find("option name OwnBook") != std::string::npos);
    CATFISH_EXPECT_TRUE(output.str().find("uciok") != std::string::npos);

    output.str("");
    output.clear();
    CATFISH_EXPECT_TRUE(session.handle_command("isready", output, diagnostics));
    CATFISH_EXPECT_EQ(output.str(), std::string("readyok\n"));

    CATFISH_EXPECT_TRUE(session.handle_command(
        "position startpos moves e2e4 e7e5 g1f3",
        output,
        diagnostics
    ));
    CATFISH_EXPECT_EQ(
        catfish::board_to_fen(session.board()),
        std::string("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    );
}

void test_uci_invalid_position_is_transactional() {
    catfish::UciSession session;
    std::ostringstream output;
    std::ostringstream diagnostics;
    const auto before = catfish::board_to_fen(session.board());

    session.handle_command("position startpos moves e2e5", output, diagnostics);
    CATFISH_EXPECT_EQ(catfish::board_to_fen(session.board()), before);
    CATFISH_EXPECT_TRUE(diagnostics.str().find("illegal move") != std::string::npos);
}

void test_uci_search_and_terminal_position() {
    catfish::UciSession session;
    std::ostringstream output;
    std::ostringstream diagnostics;

    session.handle_command("go depth 1", output, diagnostics);
    CATFISH_EXPECT_TRUE(output.str().find("info string book") != std::string::npos);
    CATFISH_EXPECT_TRUE(output.str().find("bestmove ") != std::string::npos);

    output.str("");
    output.clear();
    session.handle_command("setoption name OwnBook value false", output, diagnostics);
    session.handle_command("go depth 1", output, diagnostics);
    CATFISH_EXPECT_TRUE(output.str().find("info depth 1") != std::string::npos);

    output.str("");
    output.clear();
    session.handle_command(
        "position fen 7k/5Q2/7K/8/8/8/8/8 b - - 0 1",
        output,
        diagnostics
    );
    session.handle_command("go depth 1", output, diagnostics);
    CATFISH_EXPECT_TRUE(output.str().find("bestmove 0000") != std::string::npos);
}

} // namespace catfish_tests
