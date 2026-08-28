#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace catfish {

using Bitboard = std::uint64_t;
using Square = int;

constexpr Square kNoSquare = -1;
constexpr int kBoardSize = 64;

enum class Color : int {
    White = 0,
    Black = 1
};

enum class PieceType : int {
    Pawn = 0,
    Knight = 1,
    Bishop = 2,
    Rook = 3,
    Queen = 4,
    King = 5,
    None = 6
};

struct Piece {
    Color color{Color::White};
    PieceType type{PieceType::None};
};

enum CastlingRights : std::uint8_t {
    NoCastling = 0,
    WhiteKingSide = 1 << 0,
    WhiteQueenSide = 1 << 1,
    BlackKingSide = 1 << 2,
    BlackQueenSide = 1 << 3
};

constexpr Color opposite(Color color) {
    return color == Color::White ? Color::Black : Color::White;
}

constexpr int color_index(Color color) {
    return static_cast<int>(color);
}

constexpr int piece_index(PieceType piece) {
    return static_cast<int>(piece);
}

constexpr int file_of(Square square) {
    return square & 7;
}

constexpr int rank_of(Square square) {
    return square >> 3;
}

constexpr Square make_square(int file, int rank) {
    return rank * 8 + file;
}

char piece_to_char(Color color, PieceType piece);
PieceType char_to_piece_type(char c);
Color char_to_color(char c);
std::string square_to_string(Square square);
Square square_from_string(const std::string& text);

} // namespace catfish

