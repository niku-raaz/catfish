#include "catfish/board.hpp"

#include "catfish/bitboard.hpp"
#include "catfish/fen.hpp"

#include <sstream>

namespace catfish {

namespace {

bool attacks_along_ray(const Board& board, Square from, int file_delta, int rank_delta, Color by_color) {
    int file = file_of(from) + file_delta;
    int rank = rank_of(from) + rank_delta;
    const bool diagonal = file_delta != 0 && rank_delta != 0;
    const Bitboard attackers = diagonal
        ? (board.pieces(by_color, PieceType::Bishop) | board.pieces(by_color, PieceType::Queen))
        : (board.pieces(by_color, PieceType::Rook) | board.pieces(by_color, PieceType::Queen));
    const Bitboard occupied = board.occupancy_all();

    while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
        const Square square = make_square(file, rank);
        const Bitboard square_mask = bit(square);
        if ((occupied & square_mask) != 0) {
            return (attackers & square_mask) != 0;
        }
        file += file_delta;
        rank += rank_delta;
    }

    return false;
}

} // namespace

Board::Board() {
    init_bitboards();
    clear();
}

Board Board::start_position() {
    return board_from_fen(kStartFen);
}

void Board::clear() {
    for (auto& color_pieces : pieces_) {
        color_pieces.fill(0);
    }
    side_to_move_ = Color::White;
    castling_rights_ = NoCastling;
    en_passant_square_ = kNoSquare;
    halfmove_clock_ = 0;
    fullmove_number_ = 1;
    position_key_ = zobrist_castling(NoCastling);
}

Bitboard Board::pieces(Color color, PieceType piece) const {
    return pieces_[color_index(color)][piece_index(piece)];
}

Bitboard Board::occupancy(Color color) const {
    Bitboard out = 0;
    for (const Bitboard board : pieces_[color_index(color)]) {
        out |= board;
    }
    return out;
}

Bitboard Board::occupancy_all() const {
    return occupancy(Color::White) | occupancy(Color::Black);
}

Color Board::side_to_move() const {
    return side_to_move_;
}

std::uint8_t Board::castling_rights() const {
    return castling_rights_;
}

Square Board::en_passant_square() const {
    return en_passant_square_;
}

int Board::halfmove_clock() const {
    return halfmove_clock_;
}

int Board::fullmove_number() const {
    return fullmove_number_;
}

ZobristKey Board::position_key() const {
    return position_key_;
}

void Board::set_side_to_move(Color color) {
    if (side_to_move_ != color) {
        position_key_ ^= en_passant_hash();
        position_key_ ^= zobrist_side_to_move();
        side_to_move_ = color;
        position_key_ ^= en_passant_hash();
        return;
    }
    side_to_move_ = color;
}

void Board::set_castling_rights(std::uint8_t rights) {
    position_key_ ^= zobrist_castling(castling_rights_);
    position_key_ ^= zobrist_castling(rights);
    castling_rights_ = rights;
}

void Board::set_en_passant_square(Square square) {
    position_key_ ^= en_passant_hash();
    en_passant_square_ = square;
    position_key_ ^= en_passant_hash();
}

void Board::set_halfmove_clock(int clock) {
    halfmove_clock_ = clock;
}

void Board::set_fullmove_number(int number) {
    fullmove_number_ = number;
}

void Board::set_piece(Square square, Color color, PieceType piece) {
    clear_square_from_piece_sets(square);
    put_piece(square, color, piece);
}

void Board::remove_piece(Square square) {
    clear_square_from_piece_sets(square);
}

std::optional<Piece> Board::piece_at(Square square) const {
    const Bitboard mask = bit(square);
    for (Color color : {Color::White, Color::Black}) {
        for (int piece = 0; piece < 6; ++piece) {
            if ((pieces_[color_index(color)][piece] & mask) != 0) {
                return Piece{color, static_cast<PieceType>(piece)};
            }
        }
    }
    return std::nullopt;
}

Square Board::king_square(Color color) const {
    return lsb(pieces(color, PieceType::King));
}

UndoState Board::make_move(const Move& move) {
    const Color us = side_to_move_;
    const Color them = opposite(us);
    const Square captured_square = (move.flags & EnPassant)
        ? (us == Color::White ? move.to - 8 : move.to + 8)
        : move.to;
    const bool has_capture = move.captured != PieceType::None;

    UndoState undo{
        move,
        move.captured,
        has_capture ? captured_square : kNoSquare,
        castling_rights_,
        en_passant_square_,
        halfmove_clock_,
        fullmove_number_,
        position_key_
    };

    set_en_passant_square(kNoSquare);
    clear_piece(move.from, us, move.piece);
    if (has_capture) {
        clear_piece(captured_square, them, move.captured);
    }

    const PieceType placed_piece = move.is_promotion() ? move.promotion : move.piece;
    put_piece(move.to, us, placed_piece);

    if (move.flags & KingCastle) {
        const Square rook_from = us == Color::White ? 7 : 63;
        const Square rook_to = us == Color::White ? 5 : 61;
        clear_piece(rook_from, us, PieceType::Rook);
        put_piece(rook_to, us, PieceType::Rook);
    } else if (move.flags & QueenCastle) {
        const Square rook_from = us == Color::White ? 0 : 56;
        const Square rook_to = us == Color::White ? 3 : 59;
        clear_piece(rook_from, us, PieceType::Rook);
        put_piece(rook_to, us, PieceType::Rook);
    }

    const std::uint8_t old_castling_rights = castling_rights_;
    update_castling_rights_for_move(move, has_capture ? captured_square : kNoSquare);
    if (old_castling_rights != castling_rights_) {
        position_key_ ^= zobrist_castling(old_castling_rights);
        position_key_ ^= zobrist_castling(castling_rights_);
    }
    halfmove_clock_ = (move.piece == PieceType::Pawn || has_capture) ? 0 : halfmove_clock_ + 1;
    if (us == Color::Black) {
        ++fullmove_number_;
    }
    set_side_to_move(them);
    if (move.flags & DoublePawnPush) {
        set_en_passant_square(us == Color::White ? move.from + 8 : move.from - 8);
    }

    return undo;
}

void Board::unmake_move(const UndoState& undo) {
    const Move& move = undo.move;
    set_en_passant_square(kNoSquare);
    set_side_to_move(opposite(side_to_move_));
    const Color us = side_to_move_;

    clear_piece(move.to, us, move.is_promotion() ? move.promotion : move.piece);
    put_piece(move.from, us, move.piece);

    if (move.flags & KingCastle) {
        const Square rook_from = us == Color::White ? 7 : 63;
        const Square rook_to = us == Color::White ? 5 : 61;
        clear_piece(rook_to, us, PieceType::Rook);
        put_piece(rook_from, us, PieceType::Rook);
    } else if (move.flags & QueenCastle) {
        const Square rook_from = us == Color::White ? 0 : 56;
        const Square rook_to = us == Color::White ? 3 : 59;
        clear_piece(rook_to, us, PieceType::Rook);
        put_piece(rook_from, us, PieceType::Rook);
    }

    if (undo.captured_piece != PieceType::None && undo.captured_square != kNoSquare) {
        put_piece(undo.captured_square, opposite(us), undo.captured_piece);
    }

    castling_rights_ = undo.castling_rights;
    en_passant_square_ = undo.en_passant_square;
    halfmove_clock_ = undo.halfmove_clock;
    fullmove_number_ = undo.fullmove_number;
    position_key_ = undo.position_key;
}

bool Board::is_square_attacked(Square square, Color by_color) const {
    if ((kPawnAttacks[color_index(opposite(by_color))][square] & pieces(by_color, PieceType::Pawn)) != 0) {
        return true;
    }
    if ((kKnightAttacks[square] & pieces(by_color, PieceType::Knight)) != 0) {
        return true;
    }
    if ((kKingAttacks[square] & pieces(by_color, PieceType::King)) != 0) {
        return true;
    }

    constexpr int bishop_dirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    constexpr int rook_dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    for (const auto& dir : bishop_dirs) {
        if (attacks_along_ray(*this, square, dir[0], dir[1], by_color)) {
            return true;
        }
    }
    for (const auto& dir : rook_dirs) {
        if (attacks_along_ray(*this, square, dir[0], dir[1], by_color)) {
            return true;
        }
    }

    return false;
}

bool Board::in_check(Color color) const {
    const Square king = king_square(color);
    return king != kNoSquare && is_square_attacked(king, opposite(color));
}

std::string Board::debug_string() const {
    std::ostringstream out;
    for (int rank = 7; rank >= 0; --rank) {
        out << (rank + 1) << " ";
        for (int file = 0; file < 8; ++file) {
            const auto piece = piece_at(make_square(file, rank));
            out << (piece.has_value() ? piece_to_char(piece->color, piece->type) : '.') << ' ';
        }
        out << '\n';
    }
    out << "  a b c d e f g h\n";
    out << "side: " << (side_to_move_ == Color::White ? "white" : "black") << '\n';
    return out.str();
}

void Board::clear_square_from_piece_sets(Square square) {
    const ZobristKey old_en_passant_hash = en_passant_hash();
    const Bitboard square_mask = bit(square);
    for (int color = 0; color < 2; ++color) {
        for (int piece = 0; piece < 6; ++piece) {
            Bitboard& piece_board = pieces_[color][piece];
            if ((piece_board & square_mask) != 0) {
                piece_board &= ~square_mask;
                position_key_ ^= zobrist_piece(
                    static_cast<Color>(color),
                    static_cast<PieceType>(piece),
                    square
                );
            }
        }
    }
    position_key_ ^= old_en_passant_hash;
    position_key_ ^= en_passant_hash();
}

void Board::put_piece(Square square, Color color, PieceType piece) {
    const ZobristKey old_en_passant_hash = en_passant_hash();
    pieces_[color_index(color)][piece_index(piece)] |= bit(square);
    position_key_ ^= zobrist_piece(color, piece, square);
    position_key_ ^= old_en_passant_hash;
    position_key_ ^= en_passant_hash();
}

void Board::clear_piece(Square square, Color color, PieceType piece) {
    const ZobristKey old_en_passant_hash = en_passant_hash();
    pieces_[color_index(color)][piece_index(piece)] &= ~bit(square);
    position_key_ ^= zobrist_piece(color, piece, square);
    position_key_ ^= old_en_passant_hash;
    position_key_ ^= en_passant_hash();
}

ZobristKey Board::en_passant_hash() const {
    if (en_passant_square_ == kNoSquare) {
        return 0;
    }
    const Bitboard possible_attackers =
        kPawnAttacks[color_index(opposite(side_to_move_))][en_passant_square_]
        & pieces(side_to_move_, PieceType::Pawn);
    return possible_attackers != 0
        ? zobrist_en_passant_file(file_of(en_passant_square_))
        : 0;
}

void Board::update_castling_rights_for_move(const Move& move, Square captured_square) {
    switch (move.from) {
        case 4: castling_rights_ &= ~(WhiteKingSide | WhiteQueenSide); break;
        case 60: castling_rights_ &= ~(BlackKingSide | BlackQueenSide); break;
        case 0: castling_rights_ &= ~WhiteQueenSide; break;
        case 7: castling_rights_ &= ~WhiteKingSide; break;
        case 56: castling_rights_ &= ~BlackQueenSide; break;
        case 63: castling_rights_ &= ~BlackKingSide; break;
        default: break;
    }

    switch (captured_square) {
        case 0: castling_rights_ &= ~WhiteQueenSide; break;
        case 7: castling_rights_ &= ~WhiteKingSide; break;
        case 56: castling_rights_ &= ~BlackQueenSide; break;
        case 63: castling_rights_ &= ~BlackKingSide; break;
        default: break;
    }
}

} // namespace catfish
