#pragma once

#include "catfish/board.hpp"

#include <cstdint>
#include <vector>

namespace catfish {

struct PerftEntry {
    Move move{};
    std::uint64_t nodes{0};
};

std::uint64_t perft(Board& board, int depth);
std::vector<PerftEntry> perft_divide(Board& board, int depth);

} // namespace catfish

