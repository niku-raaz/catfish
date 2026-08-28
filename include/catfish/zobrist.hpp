#pragma once

#include "catfish/types.hpp"

#include <cstdint>

namespace catfish {

class Board;

using ZobristKey = std::uint64_t;

ZobristKey zobrist_piece(Color color, PieceType piece, Square square);
ZobristKey zobrist_castling(std::uint8_t rights);
ZobristKey zobrist_en_passant_file(int file);
ZobristKey zobrist_side_to_move();
ZobristKey compute_zobrist(const Board& board);

} // namespace catfish
