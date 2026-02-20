#include "movegen.h"

void generatePawnMoves(const Position &pos, std::vector<Move> &moves) {
    uint64_t pawns = pos.whitePawns;
    uint64_t empty = ~allPieces(pos);

    uint64_t singlePush = (pawns << 8) & empty;

    while (singlePush) {
        int to = __builtin_ctzll(singlePush);
        int from = to - 8;

        moves.push_back({from, to});

        singlePush &= singlePush - 1; // remove LSB
    }
}
uint64_t knightAttacks[64];
// pprecompute knight attacks for each square
void initKnightAttacks(){

    for (int square = 0; square < 64; square++) {
        uint64_t attacks = 0ULL;

        int rank = square / 8;
        int file = square % 8;

        int knightMoves[8][2] = {
            {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
            {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
        };

        for (int i = 0; i < 8; i++) {
            int newRank = rank + knightMoves[i][0];
            int newFile = file + knightMoves[i][1];

            if (newRank >= 0 && newRank < 8 &&
                newFile >= 0 && newFile < 8) {

                int newSquare = newRank * 8 + newFile;
                attacks |= (1ULL << newSquare);
            }
        }

        knightAttacks[square] = attacks;
    }
}

void generateKnightMoves(const Position &pos, Color side, std::vector<Move> &moves) {
    uint64_t knights = pos.whiteKnights;
    uint64_t ownPieces = whitePieces(pos);

    while (knights) {
        int from = __builtin_ctzll(knights);
        uint64_t attacks = knightAttacks[from] & ~ownPieces;

        while (attacks) {
            int to = __builtin_ctzll(attacks);
            moves.push_back({from, to});

            attacks &= attacks - 1; // remove LSB
        }

        knights &= knights - 1; // remove LSB
    }
}