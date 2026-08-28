#include "catfish/move.hpp"

#include <cctype>
#include <stdexcept>

namespace catfish {

char piece_to_char(Color color, PieceType piece) {
    char c = '.';
    switch (piece) {
        case PieceType::Pawn: c = 'p'; break;
        case PieceType::Knight: c = 'n'; break;
        case PieceType::Bishop: c = 'b'; break;
        case PieceType::Rook: c = 'r'; break;
        case PieceType::Queen: c = 'q'; break;
        case PieceType::King: c = 'k'; break;
        case PieceType::None: return '.';
    }
    return color == Color::White ? static_cast<char>(std::toupper(c)) : c;
}

PieceType char_to_piece_type(char c) {
    switch (static_cast<char>(std::tolower(c))) {
        case 'p': return PieceType::Pawn;
        case 'n': return PieceType::Knight;
        case 'b': return PieceType::Bishop;
        case 'r': return PieceType::Rook;
        case 'q': return PieceType::Queen;
        case 'k': return PieceType::King;
        default: return PieceType::None;
    }
}

Color char_to_color(char c) {
    return std::isupper(static_cast<unsigned char>(c)) ? Color::White : Color::Black;
}

std::string square_to_string(Square square) {
    if (square < 0 || square >= 64) {
        return "-";
    }
    std::string out;
    out.push_back(static_cast<char>('a' + file_of(square)));
    out.push_back(static_cast<char>('1' + rank_of(square)));
    return out;
}

Square square_from_string(const std::string& text) {
    if (text.size() != 2 || text == "-") {
        return kNoSquare;
    }
    const int file = text[0] - 'a';
    const int rank = text[1] - '1';
    if (file < 0 || file >= 8 || rank < 0 || rank >= 8) {
        return kNoSquare;
    }
    return make_square(file, rank);
}

bool Move::is_capture() const {
    return (flags & Capture) != 0;
}

bool Move::is_promotion() const {
    return (flags & Promotion) != 0;
}

bool Move::is_castle() const {
    return (flags & (KingCastle | QueenCastle)) != 0;
}

std::string move_to_string(const Move& move) {
    std::string text = square_to_string(move.from) + square_to_string(move.to);
    if (move.is_promotion()) {
        text.push_back(static_cast<char>(std::tolower(piece_to_char(Color::White, move.promotion))));
    }
    return text;
}

} // namespace catfish

