#include "search.h"
#include "eval.h"
#include <climits>

// Score for White. At depth 0 return eval; else max (white) or min (black) over legal moves.
int minimax(Position &pos, int depth) {

    if (depth == 0)
        return eval(pos);

    std::vector<Move> moves;
    Color side = pos.whiteToMove ? WHITE : BLACK;

    generatePawnMoves(pos, side, moves);
    generateKnightMoves(pos, side, moves);
    generateBishopMoves(pos, side, moves);
    generateRookMoves(pos, side, moves);
    generateQueenMoves(pos, side, moves);
    generateKingMoves(pos, side, moves);
    generateCastlingMoves(pos, side, moves);

    int best = pos.whiteToMove ? INT_MIN : INT_MAX;
    bool anyLegal = false;

    for (const Move &move : moves) {
        State previous{pos};
        makeMove(pos, move);

        Color movedSide = pos.whiteToMove ? BLACK : WHITE;
        if (isKingInCheck(pos, movedSide)) {
            undoMove(pos, previous);
            continue;
        }

        anyLegal = true;
        int v = minimax(pos, depth - 1);
        undoMove(pos, previous);

        if (pos.whiteToMove)
            best = (v > best) ? v : best;
        else
            best = (v < best) ? v : best;
    }

    if (!anyLegal)
        return eval(pos);

    return best;
}

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

        // Legality: the side that just moved must not be left in check
        Color movedSide = pos.whiteToMove ? BLACK : WHITE;
        if (isKingInCheck(pos, movedSide)) {
            undoMove(pos, previous);
            continue;
        }

        nodes += perft(pos, depth - 1);

        undoMove(pos, previous);
    }

    return nodes;
}