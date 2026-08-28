#include "catfish/perft.hpp"

#include "catfish/movegen.hpp"

namespace catfish {

namespace {

std::uint64_t perft_impl(
    Board& board,
    int depth,
    std::vector<std::vector<Move>>& pseudo_stack,
    std::vector<std::vector<Move>>& move_stack
) {
    if (depth == 0) {
        return 1;
    }

    auto& moves = move_stack[depth];
    generate_legal_moves(board, pseudo_stack[depth], moves);
    if (depth == 1) {
        return moves.size();
    }

    std::uint64_t nodes = 0;
    for (const Move& move : moves) {
        const UndoState undo = board.make_move(move);
        nodes += perft_impl(board, depth - 1, pseudo_stack, move_stack);
        board.unmake_move(undo);
    }
    return nodes;
}

} // namespace

std::uint64_t perft(Board& board, int depth) {
    if (depth <= 0) {
        return 1;
    }

    std::vector<std::vector<Move>> pseudo_stack(static_cast<std::size_t>(depth + 1));
    std::vector<std::vector<Move>> move_stack(static_cast<std::size_t>(depth + 1));
    for (std::size_t i = 0; i < move_stack.size(); ++i) {
        pseudo_stack[i].reserve(128);
        move_stack[i].reserve(128);
    }
    return perft_impl(board, depth, pseudo_stack, move_stack);
}

std::vector<PerftEntry> perft_divide(Board& board, int depth) {
    std::vector<PerftEntry> entries;
    if (depth <= 0) {
        return entries;
    }

    std::vector<Move> moves;
    generate_legal_moves(board, moves);
    entries.reserve(moves.size());
    for (const Move& move : moves) {
        const UndoState undo = board.make_move(move);
        const std::uint64_t nodes = perft(board, depth - 1);
        board.unmake_move(undo);
        entries.push_back(PerftEntry{move, nodes});
    }
    return entries;
}

} // namespace catfish
