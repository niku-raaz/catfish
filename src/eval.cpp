#include "catfish/eval.hpp"

#include "catfish/bitboard.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace catfish {

namespace {

constexpr int kMaximumPhase = 24;

struct Score {
    int middle_game{0};
    int end_game{0};

    Score& operator+=(const Score& other) {
        middle_game += other.middle_game;
        end_game += other.end_game;
        return *this;
    }
};

constexpr Score operator-(Score left, const Score& right) {
    left.middle_game -= right.middle_game;
    left.end_game -= right.end_game;
    return left;
}

constexpr Score operator*(Score score, int factor) {
    return {score.middle_game * factor, score.end_game * factor};
}

constexpr std::array<Score, 6> kPieceValues{{
    {100, 120},
    {320, 300},
    {330, 325},
    {500, 520},
    {900, 900},
    {0, 0},
}};

constexpr std::array<int, 6> kPhaseWeights{{0, 1, 1, 2, 4, 0}};

struct EvalMasks {
    std::array<Bitboard, 8> files{};
    std::array<Bitboard, 8> adjacent_files{};
    std::array<std::array<Bitboard, 64>, 2> passed_pawn{};
};

EvalMasks make_eval_masks() {
    EvalMasks masks;
    for (int file = 0; file < 8; ++file) {
        for (int rank = 0; rank < 8; ++rank) {
            masks.files[file] |= bit(make_square(file, rank));
        }
    }
    for (int file = 0; file < 8; ++file) {
        if (file > 0) {
            masks.adjacent_files[file] |= masks.files[file - 1];
        }
        if (file < 7) {
            masks.adjacent_files[file] |= masks.files[file + 1];
        }
    }
    for (Color color : {Color::White, Color::Black}) {
        for (Square square = 0; square < 64; ++square) {
            const int direction = color == Color::White ? 1 : -1;
            for (int file = std::max(0, file_of(square) - 1);
                 file <= std::min(7, file_of(square) + 1);
                 ++file) {
                for (int rank = rank_of(square) + direction;
                     rank >= 0 && rank < 8;
                     rank += direction) {
                    masks.passed_pawn[color_index(color)][square] |= bit(make_square(file, rank));
                }
            }
        }
    }
    return masks;
}

const EvalMasks& eval_masks() {
    static const EvalMasks masks = make_eval_masks();
    return masks;
}

int relative_rank(Color color, Square square) {
    return color == Color::White ? rank_of(square) : 7 - rank_of(square);
}

int center_distance(Square square) {
    const int file_distance = std::min(std::abs(file_of(square) - 3), std::abs(file_of(square) - 4));
    const int rank_distance = std::min(std::abs(rank_of(square) - 3), std::abs(rank_of(square) - 4));
    return file_distance + rank_distance;
}

Score piece_square_score(PieceType piece, Color color, Square square) {
    const int center = 6 - center_distance(square);
    const int advancement = relative_rank(color, square);
    switch (piece) {
        case PieceType::Pawn:
            return {center * 2 + advancement * 4, center + advancement * advancement * 2};
        case PieceType::Knight:
            return {center * 8 - (advancement == 0 ? 12 : 0), center * 6};
        case PieceType::Bishop:
            return {center * 4 + advancement * 2, center * 4};
        case PieceType::Rook:
            return {advancement == 6 ? 18 : 0, center * 2 + (advancement == 6 ? 12 : 0)};
        case PieceType::Queen:
            return {center * 2 - (advancement > 2 ? 4 : 0), center * 2};
        case PieceType::King: {
            const bool castled_file = file_of(square) <= 2 || file_of(square) >= 6;
            return {castled_file ? 22 : -center * 6, center * 10};
        }
        case PieceType::None:
            return {};
    }
    return {};
}

struct Activity {
    Score mobility{};
    Bitboard attacks{0};
};

int add_slider_activity(
    const Board& board,
    Color color,
    Square from,
    bool diagonal,
    bool orthogonal,
    Bitboard& attacks
) {
    constexpr int directions[8][2] = {
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1},
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}
    };
    const Bitboard own = board.occupancy(color);
    const Bitboard occupied = board.occupancy_all();
    int mobility = 0;
    for (int index = 0; index < 8; ++index) {
        if ((index < 4 && !diagonal) || (index >= 4 && !orthogonal)) {
            continue;
        }
        int file = file_of(from) + directions[index][0];
        int rank = rank_of(from) + directions[index][1];
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const Square target = make_square(file, rank);
            attacks |= bit(target);
            if ((own & bit(target)) != 0) {
                break;
            }
            ++mobility;
            if ((occupied & bit(target)) != 0) {
                break;
            }
            file += directions[index][0];
            rank += directions[index][1];
        }
    }
    return mobility;
}

Score material_and_piece_square(
    const Board& board,
    Color color,
    int& phase,
    Score& piece_square
) {
    Score material;
    for (int piece_index_value = 0; piece_index_value < 6; ++piece_index_value) {
        const auto piece = static_cast<PieceType>(piece_index_value);
        Bitboard pieces = board.pieces(color, piece);
        const int count = popcount(pieces);
        material += kPieceValues[piece_index_value] * count;
        phase += kPhaseWeights[piece_index_value] * count;
        while (pieces != 0) {
            piece_square += piece_square_score(piece, color, pop_lsb(pieces));
        }
    }
    return material;
}

Score pawn_structure(const Board& board, Color color) {
    const auto& masks = eval_masks();
    const Bitboard pawns = board.pieces(color, PieceType::Pawn);
    const Bitboard enemy_pawns = board.pieces(opposite(color), PieceType::Pawn);
    Score score;

    for (int file = 0; file < 8; ++file) {
        const int count = popcount(pawns & masks.files[file]);
        if (count > 1) {
            score += Score{-14, -20} * (count - 1);
        }
        if (count > 0 && (pawns & masks.adjacent_files[file]) == 0) {
            score += Score{-12, -10} * count;
        }
    }

    Bitboard scan = pawns;
    while (scan != 0) {
        const Square square = pop_lsb(scan);
        const int advancement = relative_rank(color, square);
        if ((enemy_pawns & masks.passed_pawn[color_index(color)][square]) == 0) {
            static constexpr int middle_bonus[8] = {0, 4, 8, 16, 28, 48, 80, 0};
            static constexpr int end_bonus[8] = {0, 8, 16, 30, 52, 86, 140, 0};
            score += {middle_bonus[advancement], end_bonus[advancement]};
        }
        if ((kPawnAttacks[color_index(opposite(color))][square] & pawns) != 0) {
            score += {7, 12};
        }
    }
    return score;
}

Activity activity(const Board& board, Color color) {
    const Bitboard own = board.occupancy(color);
    int mobility = 0;
    Bitboard attacks = 0;

    Bitboard pawns = board.pieces(color, PieceType::Pawn);
    while (pawns != 0) {
        attacks |= kPawnAttacks[color_index(color)][pop_lsb(pawns)];
    }

    Bitboard knights = board.pieces(color, PieceType::Knight);
    while (knights != 0) {
        const Bitboard targets = kKnightAttacks[pop_lsb(knights)];
        attacks |= targets;
        mobility += popcount(targets & ~own) * 4;
    }
    Bitboard bishops = board.pieces(color, PieceType::Bishop);
    while (bishops != 0) {
        mobility += add_slider_activity(
            board, color, pop_lsb(bishops), true, false, attacks
        ) * 3;
    }
    Bitboard rooks = board.pieces(color, PieceType::Rook);
    while (rooks != 0) {
        mobility += add_slider_activity(
            board, color, pop_lsb(rooks), false, true, attacks
        ) * 2;
    }
    Bitboard queens = board.pieces(color, PieceType::Queen);
    while (queens != 0) {
        mobility += add_slider_activity(
            board, color, pop_lsb(queens), true, true, attacks
        );
    }
    Bitboard kings = board.pieces(color, PieceType::King);
    if (kings != 0) {
        attacks |= kKingAttacks[lsb(kings)];
    }
    return {{mobility, mobility / 2}, attacks};
}

Score king_safety_score(const Board& board, Color color, Bitboard enemy_attacks) {
    const Square king = board.king_square(color);
    if (king == kNoSquare) {
        return {};
    }
    const Bitboard pawns = board.pieces(color, PieceType::Pawn);
    const int direction = color == Color::White ? 1 : -1;
    int shield = 0;
    for (int file = std::max(0, file_of(king) - 1);
         file <= std::min(7, file_of(king) + 1);
         ++file) {
        const int first_rank = rank_of(king) + direction;
        const int second_rank = rank_of(king) + direction * 2;
        if (first_rank >= 0 && first_rank < 8 && has_bit(pawns, make_square(file, first_rank))) {
            shield += 10;
        } else if (second_rank >= 0 && second_rank < 8
                   && has_bit(pawns, make_square(file, second_rank))) {
            shield += 5;
        } else {
            shield -= 8;
        }
    }
    const int pressure = popcount((kKingAttacks[king] | bit(king)) & enemy_attacks) * 4;
    return {shield - pressure, -pressure / 2};
}

Score positional_score(const Board& board, Color color) {
    const auto& masks = eval_masks();
    Score score;
    if (popcount(board.pieces(color, PieceType::Bishop)) >= 2) {
        score += {32, 42};
    }
    Bitboard rooks = board.pieces(color, PieceType::Rook);
    const Bitboard own_pawns = board.pieces(color, PieceType::Pawn);
    const Bitboard enemy_pawns = board.pieces(opposite(color), PieceType::Pawn);
    while (rooks != 0) {
        const Square square = pop_lsb(rooks);
        const Bitboard file = masks.files[file_of(square)];
        if ((own_pawns & file) == 0) {
            score += (enemy_pawns & file) == 0 ? Score{18, 10} : Score{10, 6};
        }
    }
    return score;
}

int taper(const Score& score, int phase) {
    return (score.middle_game * phase + score.end_game * (kMaximumPhase - phase))
        / kMaximumPhase;
}

} // namespace

EvaluationBreakdown evaluate_with_breakdown(const Board& board) {
    int raw_phase = 0;
    Score white_piece_square;
    Score black_piece_square;
    const Score white_material = material_and_piece_square(
        board, Color::White, raw_phase, white_piece_square
    );
    const Score black_material = material_and_piece_square(
        board, Color::Black, raw_phase, black_piece_square
    );
    const int phase = std::clamp(raw_phase, 0, kMaximumPhase);
    const Score material = white_material - black_material;
    const Score piece_square = white_piece_square - black_piece_square;
    const Score pawns = pawn_structure(board, Color::White)
        - pawn_structure(board, Color::Black);
    const Activity white_activity = activity(board, Color::White);
    const Activity black_activity = activity(board, Color::Black);
    const Score mobility = white_activity.mobility - black_activity.mobility;
    const Score king_safety = king_safety_score(
        board, Color::White, black_activity.attacks
    ) - king_safety_score(board, Color::Black, white_activity.attacks);
    const Score positional = positional_score(board, Color::White)
        - positional_score(board, Color::Black);
    EvaluationBreakdown breakdown;
    breakdown.material = taper(material, phase);
    breakdown.piece_square = taper(piece_square, phase);
    breakdown.pawn_structure = taper(pawns, phase);
    breakdown.mobility = taper(mobility, phase);
    breakdown.king_safety = taper(king_safety, phase);
    breakdown.positional = taper(positional, phase);
    breakdown.phase = phase;
    breakdown.score = breakdown.material + breakdown.piece_square
        + breakdown.pawn_structure + breakdown.mobility
        + breakdown.king_safety + breakdown.positional;

    // Positions near an automatic fifty-move draw should not retain a full
    // material evaluation.
    if (board.halfmove_clock() > 80) {
        breakdown.score = breakdown.score * (100 - board.halfmove_clock()) / 20;
    }
    return breakdown;
}

int evaluate(const Board& board) {
    return evaluate_with_breakdown(board).score;
}

} // namespace catfish
