#pragma once

#include "catfish/board.hpp"

#include <string>

namespace catfish {

inline constexpr const char* kStartFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

Board board_from_fen(const std::string& fen);
std::string board_to_fen(const Board& board);

} // namespace catfish

