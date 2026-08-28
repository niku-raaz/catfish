#include "catfish/movegen.hpp"

#include "catfish/bitboard.hpp"

#include <array>

namespace catfish {

namespace {

PieceType piece_type_at(const Board& board, Square square, Color color) {
    const Bitboard mask = bit(square);
    for (int piece = 0; piece < 6; ++piece) {
        if ((board.pieces(color, static_cast<PieceType>(piece)) & mask) != 0) {
            return static_cast<PieceType>(piece);
        }
    }
    return PieceType::None;
}

void add_move(
    std::vector<Move>& moves,
    Square from,
    Square to,
    PieceType piece,
    PieceType captured = PieceType::None,
    std::uint16_t flags = Quiet,
    PieceType promotion = PieceType::None
) {
    if (captured == PieceType::King) {
        return;
    }
    Move move{from, to, piece, captured, promotion, flags};
    if (captured != PieceType::None) {
        move.flags |= Capture;
    }
    moves.push_back(move);
}

void add_promotion_moves(
    std::vector<Move>& moves,
    Square from,
    Square to,
    PieceType piece,
    PieceType captured,
    std::uint16_t flags
) {
    for (PieceType promotion : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
        add_move(moves, from, to, piece, captured, flags | Promotion, promotion);
    }
}

void generate_pawn_moves(const Board& board, std::vector<Move>& moves) {
    const Color us = board.side_to_move();
    const Color them = opposite(us);
    const int direction = us == Color::White ? 8 : -8;
    const int start_rank = us == Color::White ? 1 : 6;
    const int promotion_from_rank = us == Color::White ? 6 : 1;
    Bitboard pawns = board.pieces(us, PieceType::Pawn);
    const Bitboard enemy = board.occupancy(them);
    const Bitboard occupied = board.occupancy_all();

    while (pawns != 0) {
        const Square from = pop_lsb(pawns);
        const int rank = rank_of(from);
        const Square one = from + direction;
        if (one >= 0 && one < 64 && !has_bit(occupied, one)) {
            if (rank == promotion_from_rank) {
                add_promotion_moves(moves, from, one, PieceType::Pawn, PieceType::None, Quiet);
            } else {
                add_move(moves, from, one, PieceType::Pawn);
                const Square two = from + (direction * 2);
                if (rank == start_rank && !has_bit(occupied, two)) {
                    add_move(moves, from, two, PieceType::Pawn, PieceType::None, DoublePawnPush);
                }
            }
        }

        Bitboard attacks = kPawnAttacks[color_index(us)][from] & enemy;
        while (attacks != 0) {
            const Square to = pop_lsb(attacks);
            const PieceType captured = piece_type_at(board, to, them);
            if (rank == promotion_from_rank) {
                add_promotion_moves(moves, from, to, PieceType::Pawn, captured, Capture);
            } else {
                add_move(moves, from, to, PieceType::Pawn, captured, Capture);
            }
        }

        const Square ep = board.en_passant_square();
        if (ep != kNoSquare && has_bit(kPawnAttacks[color_index(us)][from], ep)) {
            Move move{from, ep, PieceType::Pawn, PieceType::Pawn, PieceType::None, static_cast<std::uint16_t>(Capture | EnPassant)};
            moves.push_back(move);
        }
    }
}

void generate_leaper_moves(const Board& board, std::vector<Move>& moves, PieceType piece, const std::array<Bitboard, 64>& attack_table) {
    const Color us = board.side_to_move();
    const Color them = opposite(us);
    Bitboard pieces = board.pieces(us, piece);
    const Bitboard own = board.occupancy(us);
    const Bitboard enemy = board.occupancy(them);

    while (pieces != 0) {
        const Square from = pop_lsb(pieces);
        Bitboard attacks = attack_table[from] & ~own;
        while (attacks != 0) {
            const Square to = pop_lsb(attacks);
            const PieceType captured = has_bit(enemy, to) ? piece_type_at(board, to, them) : PieceType::None;
            add_move(moves, from, to, piece, captured);
        }
    }
}

void generate_slider_moves(const Board& board, std::vector<Move>& moves, PieceType piece, const int dirs[][2], int dir_count) {
    const Color us = board.side_to_move();
    const Color them = opposite(us);
    Bitboard sliders = board.pieces(us, piece);
    const Bitboard own = board.occupancy(us);
    const Bitboard enemy = board.occupancy(them);
    const Bitboard occupied = own | enemy;

    while (sliders != 0) {
        const Square from = pop_lsb(sliders);
        for (int i = 0; i < dir_count; ++i) {
            int file = file_of(from) + dirs[i][0];
            int rank = rank_of(from) + dirs[i][1];
            while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
                const Square to = make_square(file, rank);
                if (has_bit(own, to)) {
                    break;
                }
                const bool is_capture = has_bit(enemy, to);
                const PieceType captured = is_capture ? piece_type_at(board, to, them) : PieceType::None;
                add_move(moves, from, to, piece, captured);
                if (has_bit(occupied, to)) {
                    break;
                }
                file += dirs[i][0];
                rank += dirs[i][1];
            }
        }
    }
}

bool has_rook(const Board& board, Square square, Color color) {
    return has_bit(board.pieces(color, PieceType::Rook), square);
}

void generate_castling_moves(const Board& board, std::vector<Move>& moves) {
    const Color us = board.side_to_move();
    const Color them = opposite(us);
    const std::uint8_t rights = board.castling_rights();
    const Bitboard occupied = board.occupancy_all();

    if (board.in_check(us)) {
        return;
    }

    if (us == Color::White) {
        if ((rights & WhiteKingSide) && has_rook(board, 7, us)
            && !has_bit(occupied, 5) && !has_bit(occupied, 6)
            && !board.is_square_attacked(5, them) && !board.is_square_attacked(6, them)) {
            add_move(moves, 4, 6, PieceType::King, PieceType::None, KingCastle);
        }
        if ((rights & WhiteQueenSide) && has_rook(board, 0, us)
            && !has_bit(occupied, 1) && !has_bit(occupied, 2) && !has_bit(occupied, 3)
            && !board.is_square_attacked(3, them) && !board.is_square_attacked(2, them)) {
            add_move(moves, 4, 2, PieceType::King, PieceType::None, QueenCastle);
        }
    } else {
        if ((rights & BlackKingSide) && has_rook(board, 63, us)
            && !has_bit(occupied, 61) && !has_bit(occupied, 62)
            && !board.is_square_attacked(61, them) && !board.is_square_attacked(62, them)) {
            add_move(moves, 60, 62, PieceType::King, PieceType::None, KingCastle);
        }
        if ((rights & BlackQueenSide) && has_rook(board, 56, us)
            && !has_bit(occupied, 57) && !has_bit(occupied, 58) && !has_bit(occupied, 59)
            && !board.is_square_attacked(59, them) && !board.is_square_attacked(58, them)) {
            add_move(moves, 60, 58, PieceType::King, PieceType::None, QueenCastle);
        }
    }
}

} // namespace

void generate_pseudo_legal_moves(const Board& board, std::vector<Move>& moves) {
    moves.clear();
    if (moves.capacity() < 128) {
        moves.reserve(128);
    }

    generate_pawn_moves(board, moves);
    generate_leaper_moves(board, moves, PieceType::Knight, kKnightAttacks);
    generate_leaper_moves(board, moves, PieceType::King, kKingAttacks);

    constexpr int bishop_dirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    constexpr int rook_dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    constexpr int queen_dirs[8][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    generate_slider_moves(board, moves, PieceType::Bishop, bishop_dirs, 4);
    generate_slider_moves(board, moves, PieceType::Rook, rook_dirs, 4);
    generate_slider_moves(board, moves, PieceType::Queen, queen_dirs, 8);
    generate_castling_moves(board, moves);
}

std::vector<Move> generate_pseudo_legal_moves(const Board& board) {
    std::vector<Move> moves;
    generate_pseudo_legal_moves(board, moves);
    return moves;
}

void generate_legal_moves(Board& board, std::vector<Move>& pseudo, std::vector<Move>& legal) {
    const Color us = board.side_to_move();
    generate_pseudo_legal_moves(board, pseudo);
    legal.clear();
    if (legal.capacity() < pseudo.size()) {
        legal.reserve(pseudo.size());
    }

    for (const Move& move : pseudo) {
        const UndoState undo = board.make_move(move);
        if (!board.in_check(us)) {
            legal.push_back(move);
        }
        board.unmake_move(undo);
    }
}

void generate_legal_moves(Board& board, std::vector<Move>& legal) {
    std::vector<Move> pseudo;
    generate_legal_moves(board, pseudo, legal);
}

std::vector<Move> generate_legal_moves(Board& board) {
    std::vector<Move> legal;
    generate_legal_moves(board, legal);
    return legal;
}

} // namespace catfish
