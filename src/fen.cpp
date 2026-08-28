#include "catfish/fen.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace catfish {

namespace {

std::vector<std::string> split_fields(const std::string& text) {
    std::istringstream in(text);
    std::vector<std::string> fields;
    std::string field;
    while (in >> field) {
        fields.push_back(field);
    }
    return fields;
}

} // namespace

Board board_from_fen(const std::string& fen) {
    const auto fields = split_fields(fen);
    if (fields.size() != 6) {
        throw std::invalid_argument("FEN must contain six fields");
    }

    Board board;
    board.clear();

    int rank = 7;
    int file = 0;
    for (const char c : fields[0]) {
        if (c == '/') {
            if (file != 8) {
                throw std::invalid_argument("FEN rank does not contain eight files");
            }
            --rank;
            file = 0;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            file += c - '0';
            continue;
        }

        const PieceType piece = char_to_piece_type(c);
        if (piece == PieceType::None || file >= 8 || rank < 0) {
            throw std::invalid_argument("FEN contains invalid piece placement");
        }
        board.set_piece(make_square(file, rank), char_to_color(c), piece);
        ++file;
    }

    if (rank != 0 || file != 8) {
        throw std::invalid_argument("FEN piece placement is incomplete");
    }

    if (fields[1] == "w") {
        board.set_side_to_move(Color::White);
    } else if (fields[1] == "b") {
        board.set_side_to_move(Color::Black);
    } else {
        throw std::invalid_argument("FEN side to move must be w or b");
    }

    std::uint8_t rights = NoCastling;
    if (fields[2] != "-") {
        for (const char c : fields[2]) {
            switch (c) {
                case 'K': rights |= WhiteKingSide; break;
                case 'Q': rights |= WhiteQueenSide; break;
                case 'k': rights |= BlackKingSide; break;
                case 'q': rights |= BlackQueenSide; break;
                default: throw std::invalid_argument("FEN contains invalid castling right");
            }
        }
    }
    board.set_castling_rights(rights);

    board.set_en_passant_square(fields[3] == "-" ? kNoSquare : square_from_string(fields[3]));
    if (fields[3] != "-" && board.en_passant_square() == kNoSquare) {
        throw std::invalid_argument("FEN contains invalid en passant square");
    }

    board.set_halfmove_clock(std::stoi(fields[4]));
    board.set_fullmove_number(std::stoi(fields[5]));

    return board;
}

std::string board_to_fen(const Board& board) {
    std::ostringstream out;

    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            const auto piece = board.piece_at(make_square(file, rank));
            if (!piece.has_value()) {
                ++empty;
                continue;
            }
            if (empty > 0) {
                out << empty;
                empty = 0;
            }
            out << piece_to_char(piece->color, piece->type);
        }
        if (empty > 0) {
            out << empty;
        }
        if (rank > 0) {
            out << '/';
        }
    }

    out << ' ' << (board.side_to_move() == Color::White ? 'w' : 'b') << ' ';
    const std::uint8_t rights = board.castling_rights();
    if (rights == NoCastling) {
        out << '-';
    } else {
        if (rights & WhiteKingSide) {
            out << 'K';
        }
        if (rights & WhiteQueenSide) {
            out << 'Q';
        }
        if (rights & BlackKingSide) {
            out << 'k';
        }
        if (rights & BlackQueenSide) {
            out << 'q';
        }
    }

    out << ' ' << square_to_string(board.en_passant_square());
    out << ' ' << board.halfmove_clock();
    out << ' ' << board.fullmove_number();

    return out.str();
}

} // namespace catfish

