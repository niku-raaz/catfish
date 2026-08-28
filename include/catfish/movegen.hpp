#pragma once

#include "catfish/board.hpp"

#include <vector>

namespace catfish {

std::vector<Move> generate_pseudo_legal_moves(const Board& board);
std::vector<Move> generate_legal_moves(Board& board);
void generate_pseudo_legal_moves(const Board& board, std::vector<Move>& moves);
void generate_legal_moves(Board& board, std::vector<Move>& legal);
void generate_legal_moves(Board& board, std::vector<Move>& pseudo, std::vector<Move>& legal);

} // namespace catfish
