#include "search.h"

uint64_t perft(Position &pos, int depth) {

    if (depth == 0)
        return 1ULL;

    std::vector<Move> moves;

    Color side = pos.whiteToMove ? WHITE : BLACK;

    // Generate all pseudo-legal moves
    generatePawnMoves(pos, side, moves);
    generateKnightMoves(pos, side, moves);
    generateBishopMoves(pos, side, moves);
    generateRookMoves(pos, side, moves);
    generateQueenMoves(pos, side, moves);
    generateKingMoves(pos, side, moves);
    generateCastlingMoves(pos, side, moves);

    uint64_t nodes = 0;

    for (const Move &move : moves) {

        State previous {pos};

        makeMove(pos, move);

        nodes += perft(pos, depth - 1);

        undoMove(pos, previous);
    }

    return nodes;
}