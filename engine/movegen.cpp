#include "movegen.h"

const uint64_t RANK_2 = 0x000000000000FF00ULL;
const uint64_t RANK_7 = 0x00FF000000000000ULL;
const uint64_t RANK_8 = 0xFF00000000000000ULL;  // white promotion
const uint64_t RANK_1 = 0x00000000000000FFULL;    // black promotion

const uint64_t NOT_A_FILE = 0xfefefefefefefefeULL;
const uint64_t NOT_H_FILE = 0x7f7f7f7f7f7f7f7fULL;

int getCapturedPiece(const Position& pos, Color side, int square) {

    uint64_t mask = 1ULL << square;

    if (side == WHITE) {
        if (pos.blackPawns & mask) return PAWN;
        if (pos.blackKnights & mask) return KNIGHT;
        if (pos.blackBishops & mask) return BISHOP;
        if (pos.blackRooks & mask) return ROOK;
        if (pos.blackQueens & mask) return QUEEN;
        if (pos.blackKing & mask) return KING;
    } else {
        if (pos.whitePawns & mask) return PAWN;
        if (pos.whiteKnights & mask) return KNIGHT;
        if (pos.whiteBishops & mask) return BISHOP;
        if (pos.whiteRooks & mask) return ROOK;
        if (pos.whiteQueens & mask) return QUEEN;
        if (pos.whiteKing & mask) return KING;
    }

    return -1;  // no capture
}

void generatePawnMoves(const Position &pos, Color side, std::vector<Move> &moves) {

    uint64_t pawns;
    uint64_t ownPieces;
    uint64_t enemyPieces = (side == WHITE) ? blackPieces(pos) : whitePieces(pos);
    uint64_t empty = ~allPieces(pos);

    if (side == WHITE) {

        pawns = pos.whitePawns;
        ownPieces = whitePieces(pos);

        // Single push
        uint64_t singlePush = (pawns << 8) & empty;

        while (singlePush) {
            int to = __builtin_ctzll(singlePush);
            int from = to - 8;
            if (to >= 56) {  // promotion rank
                moves.push_back({from, to, PAWN, -1, false, false, QUEEN});
                moves.push_back({from, to, PAWN, -1, false, false, ROOK});
                moves.push_back({from, to, PAWN, -1, false, false, BISHOP});
                moves.push_back({from, to, PAWN, -1, false, false, KNIGHT});
            } else {
                moves.push_back({from, to, PAWN, -1, false, false, -1});
            }
            singlePush &= singlePush - 1;
        }

        // Captures
        uint64_t captureLeft  = ((pawns & NOT_A_FILE) << 7) & enemyPieces;
        uint64_t captureRight = ((pawns & NOT_H_FILE) << 9) & enemyPieces;

        while (captureLeft) {
            int to = __builtin_ctzll(captureLeft);
            int captured = getCapturedPiece(pos, side, to);
            if (to >= 56) {
                moves.push_back({to - 7, to, PAWN, captured, false, false, QUEEN});
                moves.push_back({to - 7, to, PAWN, captured, false, false, ROOK});
                moves.push_back({to - 7, to, PAWN, captured, false, false, BISHOP});
                moves.push_back({to - 7, to, PAWN, captured, false, false, KNIGHT});
            } else {
                moves.push_back({to - 7, to, PAWN, captured, false, false, -1});
            }
            captureLeft &= captureLeft - 1;
        }

        while (captureRight) {
            int to = __builtin_ctzll(captureRight);
            int captured = getCapturedPiece(pos, side, to);
            if (to >= 56) {
                moves.push_back({to - 9, to, PAWN, captured, false, false, QUEEN});
                moves.push_back({to - 9, to, PAWN, captured, false, false, ROOK});
                moves.push_back({to - 9, to, PAWN, captured, false, false, BISHOP});
                moves.push_back({to - 9, to, PAWN, captured, false, false, KNIGHT});
            } else {
                moves.push_back({to - 9, to, PAWN, captured, false, false, -1});
            }
            captureRight &= captureRight - 1;
        }

        // En passant (capture to empty ep square)
        if (pos.epSquare >= 0 && pos.epSquare < 64) {
            int to = pos.epSquare;
            if (to >= 8 && to < 56) {  // ep only on rank 3-6 for white capturing
                if (to >= 7 && (to - 7) % 8 != 7) {
                    int from = to - 7;
                    if (pos.whitePawns & (1ULL << from))
                        moves.push_back({from, to, PAWN, PAWN, false, true, -1});
                }
                if (to >= 9 && (to - 9) % 8 != 0) {
                    int from = to - 9;
                    if (pos.whitePawns & (1ULL << from))
                        moves.push_back({from, to, PAWN, PAWN, false, true, -1});
                }
            }
        }

        // Double Push (never to promotion rank)
        uint64_t startPawns = pawns & RANK_2;
        uint64_t oneStep = (startPawns << 8) & empty;
        uint64_t twoStep = (oneStep << 8) & empty;

        while (twoStep) {
            int to = __builtin_ctzll(twoStep);
            int from = to - 16;
            moves.push_back({from, to, PAWN, -1, false, false, -1});
            twoStep &= twoStep - 1;
        }
    } else {

        pawns = pos.blackPawns;
        ownPieces = blackPieces(pos);

        // Single push
        uint64_t singlePush = (pawns >> 8) & empty;

        while (singlePush) {
            int to = __builtin_ctzll(singlePush);
            int from = to + 8;
            if (to <= 7) {  // promotion rank
                moves.push_back({from, to, PAWN, -1, false, false, QUEEN});
                moves.push_back({from, to, PAWN, -1, false, false, ROOK});
                moves.push_back({from, to, PAWN, -1, false, false, BISHOP});
                moves.push_back({from, to, PAWN, -1, false, false, KNIGHT});
            } else {
                moves.push_back({from, to, PAWN, -1, false, false, -1});
            }
            singlePush &= singlePush - 1;
        }

        // Captures
        uint64_t captureLeft  = ((pawns & NOT_A_FILE) >> 9) & enemyPieces;
        uint64_t captureRight = ((pawns & NOT_H_FILE) >> 7) & enemyPieces;

        while (captureLeft) {
            int to = __builtin_ctzll(captureLeft);
            int captured = getCapturedPiece(pos, side, to);
            if (to <= 7) {
                moves.push_back({to + 9, to, PAWN, captured, false, false, QUEEN});
                moves.push_back({to + 9, to, PAWN, captured, false, false, ROOK});
                moves.push_back({to + 9, to, PAWN, captured, false, false, BISHOP});
                moves.push_back({to + 9, to, PAWN, captured, false, false, KNIGHT});
            } else {
                moves.push_back({to + 9, to, PAWN, captured, false, false, -1});
            }
            captureLeft &= captureLeft - 1;
        }

        while (captureRight) {
            int to = __builtin_ctzll(captureRight);
            int captured = getCapturedPiece(pos, side, to);
            if (to <= 7) {
                moves.push_back({to + 7, to, PAWN, captured, false, false, QUEEN});
                moves.push_back({to + 7, to, PAWN, captured, false, false, ROOK});
                moves.push_back({to + 7, to, PAWN, captured, false, false, BISHOP});
                moves.push_back({to + 7, to, PAWN, captured, false, false, KNIGHT});
            } else {
                moves.push_back({to + 7, to, PAWN, captured, false, false, -1});
            }
            captureRight &= captureRight - 1;
        }

        // En passant
        if (pos.epSquare >= 0 && pos.epSquare < 64) {
            int to = pos.epSquare;
            if (to >= 8 && to <= 55) {
                if (to + 7 <= 63 && (to + 7) % 8 != 0) {
                    int from = to + 7;
                    if (pos.blackPawns & (1ULL << from))
                        moves.push_back({from, to, PAWN, PAWN, false, true, -1});
                }
                if (to + 9 <= 63 && (to + 9) % 8 != 7) {
                    int from = to + 9;
                    if (pos.blackPawns & (1ULL << from))
                        moves.push_back({from, to, PAWN, PAWN, false, true, -1});
                }
            }
        }

        // Double Push
        uint64_t startPawns = pawns & RANK_7;
        uint64_t oneStep = (startPawns >> 8) & empty;
        uint64_t twoStep = (oneStep >> 8) & empty;

        while (twoStep) {
            int to = __builtin_ctzll(twoStep);
            int from = to + 16;
            moves.push_back({from, to, PAWN, -1, false, false, -1});
            twoStep &= twoStep - 1;
        }
    }
}
uint64_t knightAttacks[64];
// precompute knight attacks for each square
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
    uint64_t knights = (side == WHITE) ? pos.whiteKnights : pos.blackKnights;
    uint64_t ownPieces = (side == WHITE) ? whitePieces(pos) : blackPieces(pos);

    while (knights) {
        int from = __builtin_ctzll(knights);
        uint64_t attacks = knightAttacks[from] & ~ownPieces;

        while (attacks) {
            int to = __builtin_ctzll(attacks);
            int captured = getCapturedPiece(pos, side, to);
            moves.push_back({from, to, KNIGHT, captured});

            attacks &= attacks - 1; // remove LSB
        }

        knights &= knights - 1; // remove LSB
    }
}

// King Moves
uint64_t kingAttacks[64];

void initKingAttacks() {

    for (int square = 0; square < 64; square++) {

        uint64_t attacks = 0ULL;

        int rank = square / 8;
        int file = square % 8;

        int kingMoves[8][2] = {
            {1,0}, {-1,0}, {0,1}, {0,-1},
            {1,1}, {1,-1}, {-1,1}, {-1,-1}
        };

        for (int i = 0; i < 8; i++) {
            int newRank = rank + kingMoves[i][0];
            int newFile = file + kingMoves[i][1];

            if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
                int newSquare = newRank * 8 + newFile;
                attacks |= (1ULL << newSquare);
            }
        }

        kingAttacks[square] = attacks;
    }
}

void generateKingMoves(const Position &pos, Color side, std::vector<Move> &moves) {

    uint64_t king;
    uint64_t ownPieces;

    if (side == WHITE) {
        king = pos.whiteKing;
        ownPieces = whitePieces(pos);
    } else {
        king = pos.blackKing;
        ownPieces = blackPieces(pos);
    }

    int from = __builtin_ctzll(king);

    uint64_t possibleMoves = kingAttacks[from] & ~ownPieces;

    while (possibleMoves) {
        int to = __builtin_ctzll(possibleMoves);

        int captured = getCapturedPiece(pos, side, to);
        moves.push_back({from, to, KING, captured, false});
        possibleMoves &= possibleMoves - 1;
    }
}

// Castling: only if rights, no pieces between, king not in check, no square crossed in check
void generateCastlingMoves(const Position &pos, Color side, std::vector<Move> &moves) {
    uint64_t occ = allPieces(pos);
    Color enemy = (side == WHITE) ? BLACK : WHITE;

    if (side == WHITE) {
        // King must be on e1
        if (!(pos.whiteKing & (1ULL << 4))) return;
        // Kingside: f1,g1 empty; e1,f1,g1 not attacked
        if (pos.whiteCanCastleK && (pos.whiteRooks & (1ULL << 7))) {
            if (!(occ & (1ULL << 5)) && !(occ & (1ULL << 6)) &&
                !isSquareAttacked(pos, 4, enemy) && !isSquareAttacked(pos, 5, enemy) && !isSquareAttacked(pos, 6, enemy))
                moves.push_back({4, 6, KING, -1, true});
        }
        // Queenside: b1,c1,d1 empty; e1,d1,c1 not attacked
        if (pos.whiteCanCastleQ && (pos.whiteRooks & (1ULL << 0))) {
            if (!(occ & (1ULL << 1)) && !(occ & (1ULL << 2)) && !(occ & (1ULL << 3)) &&
                !isSquareAttacked(pos, 4, enemy) && !isSquareAttacked(pos, 3, enemy) && !isSquareAttacked(pos, 2, enemy))
                moves.push_back({4, 2, KING, -1, true});
        }
    } else {
        if (!(pos.blackKing & (1ULL << 60))) return;
        if (pos.blackCanCastleK && (pos.blackRooks & (1ULL << 63))) {
            if (!(occ & (1ULL << 61)) && !(occ & (1ULL << 62)) &&
                !isSquareAttacked(pos, 60, enemy) && !isSquareAttacked(pos, 61, enemy) && !isSquareAttacked(pos, 62, enemy))
                moves.push_back({60, 62, KING, -1, true});
        }
        if (pos.blackCanCastleQ && (pos.blackRooks & (1ULL << 56))) {
            if (!(occ & (1ULL << 57)) && !(occ & (1ULL << 58)) && !(occ & (1ULL << 59)) &&
                !isSquareAttacked(pos, 60, enemy) && !isSquareAttacked(pos, 59, enemy) && !isSquareAttacked(pos, 58, enemy))
                moves.push_back({60, 58, KING, -1, true});
        }
    }
}

const int rookDirections[4][2] = {
    {1, 0},   // north
    {-1, 0},  // south
    {0, 1},   // east
    {0, -1}   // west
};

const int bishopDirections[4][2] = {
    {1, 1},    // NE
    {1, -1},   // NW
    {-1, 1},   // SE
    {-1, -1}   // SW
};

void generateSlidingMoves(const Position &pos, Color side, uint64_t pieces,
                          const int directions[][2], int directionCount,  std::vector<Move> &moves,
                          PieceType pieceType) {

    uint64_t ownPieces = (side == WHITE) ? whitePieces(pos) : blackPieces(pos);
    uint64_t enemyPieces = (side == WHITE) ? blackPieces(pos) : whitePieces(pos);

    while (pieces) {

        int from = __builtin_ctzll(pieces);
        pieces &= pieces - 1;

        int startRank = from / 8;
        int startFile = from % 8;

        for (int d = 0; d < directionCount; d++) {

            int r = startRank + directions[d][0];
            int f = startFile + directions[d][1];

            while (r >= 0 && r < 8 && f >= 0 && f < 8) {

                int to = r * 8 + f;

                if (ownPieces & (1ULL << to))
                    break;
                int captured = getCapturedPiece(pos, side, to);
                moves.push_back({from, to, pieceType, captured});

                if (enemyPieces & (1ULL << to))
                    break;

                r += directions[d][0];
                f += directions[d][1];
            }
        }
    }
}

void generateRookMoves(const Position &pos, Color side, std::vector<Move> &moves) {

    uint64_t rooks = (side == WHITE) ? pos.whiteRooks : pos.blackRooks;

    generateSlidingMoves(pos, side, rooks, rookDirections, 4, moves, ROOK);
}

void generateBishopMoves(const Position &pos, Color side, std::vector<Move> &moves) {

    uint64_t bishops = (side == WHITE) ? pos.whiteBishops : pos.blackBishops;

    generateSlidingMoves(pos, side, bishops, bishopDirections, 4, moves, BISHOP);
}

void generateQueenMoves(const Position &pos, Color side, std::vector<Move> &moves) {

    uint64_t queens = (side == WHITE) ? pos.whiteQueens : pos.blackQueens;

    generateSlidingMoves(pos, side, queens, rookDirections, 4, moves, QUEEN);
    generateSlidingMoves(pos, side, queens, bishopDirections, 4, moves, QUEEN);
}

// --- Check detection ---

// Returns true if the given square is attacked by any piece of bySide.
bool isSquareAttacked(const Position &pos, int square, Color bySide) {
    uint64_t occ = allPieces(pos);
    uint64_t sqMask = 1ULL << square;
    int r = square / 8, f = square % 8;

    if (bySide == WHITE) {
        // White pawns: attack square from (square-7) and (square-9)
        if (square >= 7 && (square - 7) % 8 != 7 && (pos.whitePawns & (1ULL << (square - 7)))) return true;
        if (square >= 9 && (square - 9) % 8 != 0 && (pos.whitePawns & (1ULL << (square - 9)))) return true;
        // Knights
        if (pos.whiteKnights & knightAttacks[square]) return true;
        // King
        if (pos.whiteKing & kingAttacks[square]) return true;
        // Rooks / Queens (slide from square outward; first piece hit must be white rook or queen)
        for (int d = 0; d < 4; d++) {
            int nr = r + rookDirections[d][0], nf = f + rookDirections[d][1];
            while (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
                int to = nr * 8 + nf;
                uint64_t toMask = 1ULL << to;
                if (pos.whiteRooks & toMask || pos.whiteQueens & toMask) return true;
                if (occ & toMask) break;
                nr += rookDirections[d][0]; nf += rookDirections[d][1];
            }
        }
        // Bishops / Queens
        for (int d = 0; d < 4; d++) {
            int nr = r + bishopDirections[d][0], nf = f + bishopDirections[d][1];
            while (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
                int to = nr * 8 + nf;
                uint64_t toMask = 1ULL << to;
                if (pos.whiteBishops & toMask || pos.whiteQueens & toMask) return true;
                if (occ & toMask) break;
                nr += bishopDirections[d][0]; nf += bishopDirections[d][1];
            }
        }
    } else {
        // Black pawns: attack square from (square+7) and (square+9)
        if (square + 7 <= 63 && (square + 7) % 8 != 0 && (pos.blackPawns & (1ULL << (square + 7)))) return true;
        if (square + 9 <= 63 && (square + 9) % 8 != 7 && (pos.blackPawns & (1ULL << (square + 9)))) return true;
        if (pos.blackKnights & knightAttacks[square]) return true;
        if (pos.blackKing & kingAttacks[square]) return true;
        for (int d = 0; d < 4; d++) {
            int nr = r + rookDirections[d][0], nf = f + rookDirections[d][1];
            while (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
                int to = nr * 8 + nf;
                uint64_t toMask = 1ULL << to;
                if (pos.blackRooks & toMask || pos.blackQueens & toMask) return true;
                if (occ & toMask) break;
                nr += rookDirections[d][0]; nf += rookDirections[d][1];
            }
        }
        for (int d = 0; d < 4; d++) {
            int nr = r + bishopDirections[d][0], nf = f + bishopDirections[d][1];
            while (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
                int to = nr * 8 + nf;
                uint64_t toMask = 1ULL << to;
                if (pos.blackBishops & toMask || pos.blackQueens & toMask) return true;
                if (occ & toMask) break;
                nr += bishopDirections[d][0]; nf += bishopDirections[d][1];
            }
        }
    }
    return false;
}

// Returns true if the king of the given side is in check.
bool isKingInCheck(const Position &pos, Color side) {
    int kingSquare;
    if (side == WHITE) {
        if (!pos.whiteKing) return false; // no king (shouldn't happen)
        kingSquare = __builtin_ctzll(pos.whiteKing);
    } else {
        if (!pos.blackKing) return false;
        kingSquare = __builtin_ctzll(pos.blackKing);
    }
    Color attacker = (side == WHITE) ? BLACK : WHITE;
    return isSquareAttacked(pos, kingSquare, attacker);
}