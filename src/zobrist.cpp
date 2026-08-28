#include "catfish/zobrist.hpp"

#include "catfish/bitboard.hpp"
#include "catfish/board.hpp"

#include <array>
#include <random>

namespace catfish {

namespace {

struct ZobristTables {
    std::array<std::array<std::array<ZobristKey, 64>, 6>, 2> pieces{};
    std::array<ZobristKey, 16> castling{};
    std::array<ZobristKey, 8> en_passant_file{};
    ZobristKey black_to_move{};
};

ZobristTables make_tables() {
    std::mt19937_64 rng(0xC47F15A5ULL);
    ZobristTables tables;
    for (auto& color : tables.pieces) {
        for (auto& piece : color) {
            for (auto& square : piece) {
                square = rng();
            }
        }
    }
    for (auto& value : tables.castling) {
        value = rng();
    }
    for (auto& value : tables.en_passant_file) {
        value = rng();
    }
    tables.black_to_move = rng();
    return tables;
}

const ZobristTables& tables() {
    static const ZobristTables instance = make_tables();
    return instance;
}

} // namespace

ZobristKey zobrist_piece(Color color, PieceType piece, Square square) {
    return tables().pieces[color_index(color)][piece_index(piece)][square];
}

ZobristKey zobrist_castling(std::uint8_t rights) {
    return tables().castling[rights & 0xF];
}

ZobristKey zobrist_en_passant_file(int file) {
    return tables().en_passant_file[file];
}

ZobristKey zobrist_side_to_move() {
    return tables().black_to_move;
}

ZobristKey compute_zobrist(const Board& board) {
    ZobristKey key = 0;
    const auto& zobrist = tables();

    for (Color color : {Color::White, Color::Black}) {
        for (int piece = 0; piece < 6; ++piece) {
            Bitboard pieces = board.pieces(color, static_cast<PieceType>(piece));
            while (pieces != 0) {
                const Square square = pop_lsb(pieces);
                key ^= zobrist.pieces[color_index(color)][piece][square];
            }
        }
    }

    key ^= zobrist.castling[board.castling_rights() & 0xF];
    if (board.en_passant_square() != kNoSquare) {
        const Bitboard possible_attackers =
            kPawnAttacks[color_index(opposite(board.side_to_move()))][board.en_passant_square()]
            & board.pieces(board.side_to_move(), PieceType::Pawn);
        if (possible_attackers != 0) {
            key ^= zobrist.en_passant_file[file_of(board.en_passant_square())];
        }
    }
    if (board.side_to_move() == Color::Black) {
        key ^= zobrist.black_to_move;
    }

    return key;
}

} // namespace catfish
