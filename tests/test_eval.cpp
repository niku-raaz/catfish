#include "catfish/eval.hpp"
#include "catfish/fen.hpp"

#include "test_support.hpp"

namespace catfish_tests {

void test_tapered_evaluation() {
    const auto start = catfish::Board::start_position();
    const auto start_breakdown = catfish::evaluate_with_breakdown(start);
    CATFISH_EXPECT_EQ(start_breakdown.score, 0);
    CATFISH_EXPECT_EQ(start_breakdown.phase, 24);

    const auto ending = catfish::board_from_fen("8/5pk1/6p1/3p4/3P1P2/5KP1/8/8 w - - 0 35");
    CATFISH_EXPECT_EQ(catfish::evaluate_with_breakdown(ending).phase, 0);

    const auto white_queen = catfish::board_from_fen("7k/8/8/8/8/8/8/Q5K1 w - - 0 1");
    CATFISH_EXPECT_TRUE(catfish::evaluate(white_queen) > 800);
}

} // namespace catfish_tests
