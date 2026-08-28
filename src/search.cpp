#include "catfish/search.hpp"

#include "catfish/bitboard.hpp"
#include "catfish/eval.hpp"
#include "catfish/movegen.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace catfish {

namespace {

constexpr int kMateScore = 100000;
constexpr int kMateThreshold = 99000;
constexpr int kInfinity = 1000000;
constexpr int kMaximumPly = 128;

enum class Bound : std::uint8_t {
    None,
    Exact,
    Lower,
    Upper
};

struct TranspositionEntry {
    ZobristKey key{0};
    Move best_move{};
    int score{0};
    int static_evaluation{0};
    std::int16_t depth{-1};
    Bound bound{Bound::None};
    std::uint8_t generation{0};
};

bool moves_equal(const Move& left, const Move& right) {
    return left.from == right.from
        && left.to == right.to
        && left.promotion == right.promotion;
}

int piece_value(PieceType piece) {
    switch (piece) {
        case PieceType::Pawn: return kPawnValue;
        case PieceType::Knight: return kKnightValue;
        case PieceType::Bishop: return kBishopValue;
        case PieceType::Rook: return kRookValue;
        case PieceType::Queen: return kQueenValue;
        case PieceType::King: return kKingValue;
        case PieceType::None: return 0;
    }
    return 0;
}

int score_to_table(int score, int ply) {
    if (score >= kMateThreshold) {
        return score + ply;
    }
    if (score <= -kMateThreshold) {
        return score - ply;
    }
    return score;
}

int score_from_table(int score, int ply) {
    if (score >= kMateThreshold) {
        return score - ply;
    }
    if (score <= -kMateThreshold) {
        return score + ply;
    }
    return score;
}

bool is_insufficient_material(const Board& board) {
    if ((board.pieces(Color::White, PieceType::Pawn)
         | board.pieces(Color::Black, PieceType::Pawn)
         | board.pieces(Color::White, PieceType::Rook)
         | board.pieces(Color::Black, PieceType::Rook)
         | board.pieces(Color::White, PieceType::Queen)
         | board.pieces(Color::Black, PieceType::Queen)) != 0) {
        return false;
    }

    const int white_knights = popcount(board.pieces(Color::White, PieceType::Knight));
    const int black_knights = popcount(board.pieces(Color::Black, PieceType::Knight));
    const Bitboard bishops = board.pieces(Color::White, PieceType::Bishop)
        | board.pieces(Color::Black, PieceType::Bishop);
    const int minor_count = white_knights + black_knights + popcount(bishops);
    if (minor_count <= 1) {
        return true;
    }
    if (white_knights != 0 || black_knights != 0) {
        return false;
    }

    bool saw_light_square = false;
    bool saw_dark_square = false;
    Bitboard scan = bishops;
    while (scan != 0) {
        const Square square = pop_lsb(scan);
        if (((file_of(square) + rank_of(square)) & 1) == 0) {
            saw_dark_square = true;
        } else {
            saw_light_square = true;
        }
    }
    return !(saw_light_square && saw_dark_square);
}

} // namespace

struct SearchEngine::Impl {
    struct EvaluationCacheEntry {
        ZobristKey key{0};
        int white_score{0};
        bool valid{false};
    };

    std::vector<TranspositionEntry> table{};
    std::array<EvaluationCacheEntry, 65536> evaluation_cache{};
    std::uint8_t generation{0};
    std::array<std::array<Move, 2>, kMaximumPly> killers{};
    std::array<std::array<int, 64>, 64> history_scores{};
    std::array<std::array<Move, kMaximumPly>, kMaximumPly> pv{};
    std::array<int, kMaximumPly> pv_length{};

    SearchResult result{};
    std::vector<ZobristKey> position_history{};
    std::vector<std::vector<Move>> pseudo_stack{};
    std::vector<std::vector<Move>> move_stack{};

    explicit Impl(std::size_t hash_size_mb) {
        resize(hash_size_mb);
    }

    void resize(std::size_t hash_size_mb) {
        const std::size_t bytes = std::max<std::size_t>(1, hash_size_mb) * 1024ULL * 1024ULL;
        const std::size_t count = std::max<std::size_t>(1, bytes / sizeof(TranspositionEntry));
        table.assign(count, {});
    }

    void clear() {
        std::fill(table.begin(), table.end(), TranspositionEntry{});
        for (auto& row : history_scores) {
            row.fill(0);
        }
        killers = {};
        evaluation_cache = {};
        ++generation;
    }

    int side_relative_evaluation(const Board& board) {
        EvaluationCacheEntry& cached =
            evaluation_cache[board.position_key() % evaluation_cache.size()];
        if (!cached.valid || cached.key != board.position_key()) {
            cached.key = board.position_key();
            cached.white_score = evaluate(board);
            cached.valid = true;
        }
        return (board.side_to_move() == Color::White
            ? cached.white_score
            : -cached.white_score) + 10;
    }

    TranspositionEntry* probe(ZobristKey key) {
        TranspositionEntry& entry = table[key % table.size()];
        return entry.bound != Bound::None && entry.key == key ? &entry : nullptr;
    }

    void store(
        ZobristKey key,
        int depth,
        int score,
        int static_evaluation,
        Bound bound,
        const Move& best_move,
        int ply
    ) {
        TranspositionEntry& entry = table[key % table.size()];
        const bool replace = entry.bound == Bound::None
            || entry.key == key
            || entry.generation != generation
            || depth >= entry.depth;
        if (!replace) {
            return;
        }
        entry.key = key;
        entry.best_move = best_move;
        entry.score = score_to_table(score, ply);
        entry.static_evaluation = static_evaluation;
        entry.depth = static_cast<std::int16_t>(depth);
        entry.bound = bound;
        entry.generation = generation;
    }

    bool is_repetition(const Board& board) const {
        if (position_history.empty()) {
            return false;
        }
        const std::size_t reversible_positions = static_cast<std::size_t>(board.halfmove_clock()) + 1;
        const std::size_t begin = position_history.size() > reversible_positions
            ? position_history.size() - reversible_positions
            : 0;
        int occurrences = 0;
        for (std::size_t index = begin; index < position_history.size(); ++index) {
            if (position_history[index] == board.position_key() && ++occurrences >= 3) {
                return true;
            }
        }
        return false;
    }

    bool is_draw(const Board& board) const {
        return board.halfmove_clock() >= 100
            || is_repetition(board)
            || is_insufficient_material(board);
    }

    int move_order_score(const Move& move, const Move* table_move, int ply) const {
        if (table_move != nullptr && moves_equal(move, *table_move)) {
            return 2'000'000;
        }
        if (move.is_capture()) {
            return 1'000'000
                + piece_value(move.captured) * 16
                - piece_value(move.piece);
        }
        if (move.is_promotion()) {
            return 900'000 + piece_value(move.promotion);
        }
        if (ply < kMaximumPly) {
            if (moves_equal(move, killers[ply][0])) {
                return 800'000;
            }
            if (moves_equal(move, killers[ply][1])) {
                return 799'000;
            }
        }
        return history_scores[move.from][move.to] + (move.is_castle() ? 500 : 0);
    }

    void order_moves(std::vector<Move>& moves, const Move* table_move, int ply) const {
        std::stable_sort(moves.begin(), moves.end(), [&](const Move& left, const Move& right) {
            return move_order_score(left, table_move, ply)
                > move_order_score(right, table_move, ply);
        });
    }

    void record_quiet_cutoff(const Move& move, int ply, int depth) {
        if (move.is_capture() || move.is_promotion() || ply >= kMaximumPly) {
            return;
        }
        if (!moves_equal(move, killers[ply][0])) {
            killers[ply][1] = killers[ply][0];
            killers[ply][0] = move;
        }
        int& history = history_scores[move.from][move.to];
        history = std::min(200'000, history + depth * depth);
    }

    void prepare_stacks() {
        pseudo_stack.assign(kMaximumPly, {});
        move_stack.assign(kMaximumPly, {});
        for (int ply = 0; ply < kMaximumPly; ++ply) {
            pseudo_stack[ply].reserve(128);
            move_stack[ply].reserve(128);
            pv_length[ply] = ply;
        }
    }

    int quiescence(Board& board, int alpha, int beta, int ply) {
        ++result.nodes;
        ++result.quiescence_nodes;
        result.selective_depth = std::max(result.selective_depth, ply);
        pv_length[ply] = ply;

        if (is_draw(board)) {
            return 0;
        }
        if (ply >= kMaximumPly - 1) {
            return side_relative_evaluation(board);
        }

        auto& moves = move_stack[ply];
        generate_legal_moves(board, pseudo_stack[ply], moves);
        const bool in_check = board.in_check(board.side_to_move());
        if (moves.empty()) {
            return in_check ? -kMateScore + ply : 0;
        }

        if (!in_check) {
            const int stand_pat = side_relative_evaluation(board);
            if (stand_pat >= beta) {
                return stand_pat;
            }
            alpha = std::max(alpha, stand_pat);
            moves.erase(
                std::remove_if(moves.begin(), moves.end(), [](const Move& move) {
                    return !move.is_capture() && !move.is_promotion();
                }),
                moves.end()
            );
        }

        order_moves(moves, nullptr, ply);
        for (const Move& move : moves) {
            const UndoState undo = board.make_move(move);
            position_history.push_back(board.position_key());
            const int score = -quiescence(board, -beta, -alpha, ply + 1);
            position_history.pop_back();
            board.unmake_move(undo);

            if (score >= beta) {
                return score;
            }
            if (score > alpha) {
                alpha = score;
                pv[ply][ply] = move;
                for (int index = ply + 1; index < pv_length[ply + 1]; ++index) {
                    pv[ply][index] = pv[ply + 1][index];
                }
                pv_length[ply] = pv_length[ply + 1];
            }
        }
        return alpha;
    }

    int alpha_beta(Board& board, int depth, int alpha, int beta, int ply) {
        if (depth <= 0) {
            return quiescence(board, alpha, beta, ply);
        }

        ++result.nodes;
        result.selective_depth = std::max(result.selective_depth, ply);
        pv_length[ply] = ply;

        if (is_draw(board)) {
            return 0;
        }
        if (ply >= kMaximumPly - 1) {
            return side_relative_evaluation(board);
        }

        const int original_alpha = alpha;
        const ZobristKey key = board.position_key();
        TranspositionEntry* entry = probe(key);
        Move table_move{};
        const Move* table_move_pointer = nullptr;
        if (entry != nullptr) {
            ++result.transposition_hits;
            table_move = entry->best_move;
            table_move_pointer = &table_move;
            if (entry->depth >= depth) {
                const int table_score = score_from_table(entry->score, ply);
                if (entry->bound == Bound::Exact
                    || (entry->bound == Bound::Lower && table_score >= beta)
                    || (entry->bound == Bound::Upper && table_score <= alpha)) {
                    return table_score;
                }
            }
        }

        auto& moves = move_stack[ply];
        generate_legal_moves(board, pseudo_stack[ply], moves);
        if (moves.empty()) {
            return board.in_check(board.side_to_move()) ? -kMateScore + ply : 0;
        }
        order_moves(moves, table_move_pointer, ply);

        const int static_evaluation = entry != nullptr
            ? entry->static_evaluation
            : side_relative_evaluation(board);
        int best_score = -kInfinity;
        Move best_move{};
        int move_number = 0;

        for (const Move& move : moves) {
            const UndoState undo = board.make_move(move);
            position_history.push_back(board.position_key());

            int score;
            if (move_number == 0) {
                score = -alpha_beta(board, depth - 1, -beta, -alpha, ply + 1);
            } else {
                score = -alpha_beta(board, depth - 1, -alpha - 1, -alpha, ply + 1);
                if (score > alpha && score < beta) {
                    score = -alpha_beta(board, depth - 1, -beta, -alpha, ply + 1);
                }
            }

            position_history.pop_back();
            board.unmake_move(undo);
            ++move_number;

            if (score > best_score) {
                best_score = score;
                best_move = move;
            }
            if (score > alpha) {
                alpha = score;
                pv[ply][ply] = move;
                for (int index = ply + 1; index < pv_length[ply + 1]; ++index) {
                    pv[ply][index] = pv[ply + 1][index];
                }
                pv_length[ply] = pv_length[ply + 1];
            }
            if (alpha >= beta) {
                record_quiet_cutoff(move, ply, depth);
                break;
            }
        }

        const Bound bound = best_score >= beta
            ? Bound::Lower
            : (best_score <= original_alpha ? Bound::Upper : Bound::Exact);
        store(key, depth, best_score, static_evaluation, bound, best_move, ply);
        return best_score;
    }

    int root_search(Board& board, int depth) {
        pv_length[0] = 0;
        auto& moves = move_stack[0];
        generate_legal_moves(board, pseudo_stack[0], moves);
        if (moves.empty()) {
            result.best_move.reset();
            result.principal_variation.clear();
            return board.in_check(board.side_to_move()) ? -kMateScore : 0;
        }

        TranspositionEntry* entry = probe(board.position_key());
        const Move* table_move = entry != nullptr ? &entry->best_move : nullptr;
        order_moves(moves, table_move, 0);

        int alpha = -kInfinity;
        const int beta = kInfinity;
        int best_score = -kInfinity;
        Move best_move = moves.front();
        int move_number = 0;

        for (const Move& move : moves) {
            const UndoState undo = board.make_move(move);
            position_history.push_back(board.position_key());

            int score;
            if (move_number == 0) {
                score = -alpha_beta(board, depth - 1, -beta, -alpha, 1);
            } else {
                score = -alpha_beta(board, depth - 1, -alpha - 1, -alpha, 1);
                if (score > alpha) {
                    score = -alpha_beta(board, depth - 1, -beta, -alpha, 1);
                }
            }

            position_history.pop_back();
            board.unmake_move(undo);
            ++move_number;

            if (score > best_score) {
                best_score = score;
                best_move = move;
            }
            if (score > alpha) {
                alpha = score;
                pv[0][0] = move;
                for (int index = 1; index < pv_length[1]; ++index) {
                    pv[0][index] = pv[1][index];
                }
                pv_length[0] = pv_length[1];
            }
        }

        result.best_move = best_move;
        result.principal_variation.assign(pv[0].begin(), pv[0].begin() + pv_length[0]);
        store(
            board.position_key(),
            depth,
            best_score,
            side_relative_evaluation(board),
            Bound::Exact,
            best_move,
            0
        );
        return best_score;
    }

    SearchResult search(
        Board& board,
        int requested_depth,
        const std::vector<ZobristKey>& game_history
    ) {
        result = {};
        prepare_stacks();
        ++generation;
        position_history = game_history;
        if (position_history.empty() || position_history.back() != board.position_key()) {
            position_history.push_back(board.position_key());
        }

        if (is_draw(board)) {
            result.score = 0;
            return result;
        }

        const int maximum_depth = std::max(0, requested_depth);
        if (maximum_depth == 0) {
            result.score = quiescence(board, -kInfinity, kInfinity, 0);
            return result;
        }

        for (int depth = 1; depth <= maximum_depth; ++depth) {
            result.score = root_search(board, depth);
            result.depth = depth;
            if (!result.best_move.has_value()) {
                break;
            }
        }
        return result;
    }
};

SearchEngine::SearchEngine(std::size_t hash_size_mb)
    : impl_(std::make_unique<Impl>(hash_size_mb)) {}

SearchEngine::~SearchEngine() = default;
SearchEngine::SearchEngine(SearchEngine&&) noexcept = default;
SearchEngine& SearchEngine::operator=(SearchEngine&&) noexcept = default;

void SearchEngine::clear() {
    impl_->clear();
}

void SearchEngine::set_hash_size(std::size_t hash_size_mb) {
    impl_->resize(hash_size_mb);
}

SearchResult SearchEngine::search(
    Board& board,
    int depth,
    const std::vector<ZobristKey>& game_history
) {
    return impl_->search(board, depth, game_history);
}

SearchResult search_best_move(
    Board& board,
    int depth,
    const std::vector<ZobristKey>& game_history
) {
    SearchEngine engine;
    return engine.search(board, depth, game_history);
}

} // namespace catfish
