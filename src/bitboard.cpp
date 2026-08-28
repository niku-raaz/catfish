#include "catfish/bitboard.hpp"

#include <bit>
#include <sstream>

namespace catfish {

std::array<Bitboard, 64> kKnightAttacks{};
std::array<Bitboard, 64> kKingAttacks{};
std::array<std::array<Bitboard, 64>, 2> kPawnAttacks{};

namespace {

Bitboard attacks_from_offsets(Square square, const int offsets[][2], int count) {
    Bitboard attacks = 0;
    const int file = file_of(square);
    const int rank = rank_of(square);

    for (int i = 0; i < count; ++i) {
        const int next_file = file + offsets[i][0];
        const int next_rank = rank + offsets[i][1];
        if (next_file >= 0 && next_file < 8 && next_rank >= 0 && next_rank < 8) {
            attacks |= bit(make_square(next_file, next_rank));
        }
    }

    return attacks;
}

} // namespace

int popcount(Bitboard board) {
    return std::popcount(board);
}

Square lsb(Bitboard board) {
    return board == 0 ? kNoSquare : std::countr_zero(board);
}

Square pop_lsb(Bitboard& board) {
    const Square square = lsb(board);
    if (square != kNoSquare) {
        board &= board - 1;
    }
    return square;
}

std::string bitboard_to_string(Bitboard board) {
    std::ostringstream out;
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            out << (has_bit(board, make_square(file, rank)) ? "1 " : ". ");
        }
        out << '\n';
    }
    return out.str();
}

void init_bitboards() {
    static bool initialized = false;
    if (initialized) {
        return;
    }

    constexpr int knight_offsets[8][2] = {
        {1, 2}, {2, 1}, {2, -1}, {1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
    };
    constexpr int king_offsets[8][2] = {
        {1, 1}, {1, 0}, {1, -1}, {0, -1},
        {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}
    };

    for (Square square = 0; square < 64; ++square) {
        kKnightAttacks[square] = attacks_from_offsets(square, knight_offsets, 8);
        kKingAttacks[square] = attacks_from_offsets(square, king_offsets, 8);

        Bitboard white_pawn = 0;
        Bitboard black_pawn = 0;
        const int file = file_of(square);
        const int rank = rank_of(square);
        if (rank < 7) {
            if (file > 0) {
                white_pawn |= bit(make_square(file - 1, rank + 1));
            }
            if (file < 7) {
                white_pawn |= bit(make_square(file + 1, rank + 1));
            }
        }
        if (rank > 0) {
            if (file > 0) {
                black_pawn |= bit(make_square(file - 1, rank - 1));
            }
            if (file < 7) {
                black_pawn |= bit(make_square(file + 1, rank - 1));
            }
        }
        kPawnAttacks[color_index(Color::White)][square] = white_pawn;
        kPawnAttacks[color_index(Color::Black)][square] = black_pawn;
    }

    initialized = true;
}

} // namespace catfish

