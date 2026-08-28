#pragma once

#include "catfish/types.hpp"

#include <string>

namespace catfish {

enum MoveFlag : std::uint16_t {
    Quiet = 0,
    Capture = 1 << 0,
    DoublePawnPush = 1 << 1,
    KingCastle = 1 << 2,
    QueenCastle = 1 << 3,
    EnPassant = 1 << 4,
    Promotion = 1 << 5
};

struct Move {
    Square from{kNoSquare};
    Square to{kNoSquare};
    PieceType piece{PieceType::None};
    PieceType captured{PieceType::None};
    PieceType promotion{PieceType::None};
    std::uint16_t flags{Quiet};

    bool is_capture() const;
    bool is_promotion() const;
    bool is_castle() const;
};

std::string move_to_string(const Move& move);

} // namespace catfish

