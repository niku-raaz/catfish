#include "board.h"
#include<iostream>

void setStartPosition(Position &pos) {
    pos.whitePawns   = 0x000000000000FF00ULL;
    pos.blackPawns   = 0x00FF000000000000ULL;

    pos.whiteRooks   = 0x0000000000000081ULL;
    pos.blackRooks   = 0x8100000000000000ULL;

    pos.whiteKnights = 0x0000000000000042ULL;
    pos.blackKnights = 0x4200000000000000ULL;

    pos.whiteBishops = 0x0000000000000024ULL;
    pos.blackBishops = 0x2400000000000000ULL;

    pos.whiteQueens  = 0x0000000000000008ULL;
    pos.blackQueens  = 0x0800000000000000ULL;

    pos.whiteKing    = 0x0000000000000010ULL;
    pos.blackKing    = 0x1000000000000000ULL;

    pos.whiteToMove = true;
    pos.whiteCanCastleK = pos.whiteCanCastleQ = true;
    pos.blackCanCastleK = pos.blackCanCastleQ = true;
    pos.epSquare = -1;
}

uint64_t whitePieces(const Position &pos) {
    return pos.whitePawns | pos.whiteKnights | pos.whiteBishops | pos.whiteRooks | pos.whiteQueens | pos.whiteKing;
}

uint64_t blackPieces(const Position &pos) {
    return pos.blackPawns | pos.blackKnights | pos.blackBishops | pos.blackRooks | pos.blackQueens | pos.blackKing;
}

uint64_t allPieces(const Position &pos) {
    return whitePieces(pos) | blackPieces(pos);
}

void printBoard(const Position &pos) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            uint64_t mask = 1ULL << square;

            if (pos.whitePawns & mask) std::cout << "P ";
            else if(pos.whiteBishops & mask) std::cout << "B ";
            else if(pos.whiteRooks & mask) std::cout << "R ";
            else if(pos.whiteKnights & mask) std::cout << "N ";
            else if(pos.whiteQueens & mask) std::cout << "Q ";
            else if(pos.whiteKing & mask) std::cout << "K ";
            else if(pos.blackPawns & mask) std::cout << "p ";
            else if(pos.blackBishops & mask) std::cout << "b ";
            else if(pos.blackRooks & mask) std::cout << "r ";
            else if(pos.blackKnights & mask) std::cout << "n ";
            else if(pos.blackQueens & mask) std::cout << "q ";
            else if(pos.blackKing & mask) std::cout << "k ";
            else std::cout << ". ";
        }
        std::cout << "\n";
    }
}

// moving
void makeMove(Position &pos, const Move &move) {

    uint64_t fromMask = 1ULL << move.from;
    uint64_t toMask   = 1ULL << move.to;

    // En passant target for the *next* position (only set after a double push)
    int newEpSquare = -1;
    if (move.piece == PAWN) {
        int delta = move.to - move.from;
        if (delta == 16 || delta == -16)  // double push
            newEpSquare = pos.whiteToMove ? (move.to - 8) : (move.to + 8);
    }

    uint64_t* pieceBoard = nullptr;

    // Identify correct bitboard pointer for the moving piece
    if (pos.whiteToMove) {
        switch (move.piece) {
            case PAWN:   pieceBoard = &pos.whitePawns; break;
            case KNIGHT: pieceBoard = &pos.whiteKnights; break;
            case BISHOP: pieceBoard = &pos.whiteBishops; break;
            case ROOK:   pieceBoard = &pos.whiteRooks; break;
            case QUEEN:  pieceBoard = &pos.whiteQueens; break;
            case KING:   pieceBoard = &pos.whiteKing; break;
        }
    } else {
        switch (move.piece) {
            case PAWN:   pieceBoard = &pos.blackPawns; break;
            case KNIGHT: pieceBoard = &pos.blackKnights; break;
            case BISHOP: pieceBoard = &pos.blackBishops; break;
            case ROOK:   pieceBoard = &pos.blackRooks; break;
            case QUEEN:  pieceBoard = &pos.blackQueens; break;
            case KING:   pieceBoard = &pos.blackKing; break;
        }
    }

    // Remove moving piece from source
    *pieceBoard &= ~fromMask;

    // Capture removal (en passant: captured pawn is on a different square)
    if (move.isEnPassant) {
        int capSquare = pos.whiteToMove ? (move.to - 8) : (move.to + 8);
        uint64_t capMask = 1ULL << capSquare;
        if (pos.whiteToMove)
            pos.blackPawns &= ~capMask;
        else
            pos.whitePawns &= ~capMask;
    } else if (move.captured != -1) {
        uint64_t* capturedBoard = nullptr;
        uint64_t removeMask = toMask;
        if (pos.whiteToMove) {
            switch (move.captured) {
                case PAWN:   capturedBoard = &pos.blackPawns; break;
                case KNIGHT: capturedBoard = &pos.blackKnights; break;
                case BISHOP: capturedBoard = &pos.blackBishops; break;
                case ROOK:   capturedBoard = &pos.blackRooks; break;
                case QUEEN:  capturedBoard = &pos.blackQueens; break;
                case KING:   capturedBoard = &pos.blackKing; break;
            }
        } else {
            switch (move.captured) {
                case PAWN:   capturedBoard = &pos.whitePawns; break;
                case KNIGHT: capturedBoard = &pos.whiteKnights; break;
                case BISHOP: capturedBoard = &pos.whiteBishops; break;
                case ROOK:   capturedBoard = &pos.whiteRooks; break;
                case QUEEN:  capturedBoard = &pos.whiteQueens; break;
                case KING:   capturedBoard = &pos.whiteKing; break;
            }
        }
        if (capturedBoard)
            *capturedBoard &= ~removeMask;
    }

    // Place piece on destination (promotion: place promoted piece, not pawn)
    if (move.promotion >= KNIGHT && move.promotion <= QUEEN) {
        uint64_t* promoBoard = nullptr;
        if (pos.whiteToMove) {
            switch (move.promotion) {
                case KNIGHT: promoBoard = &pos.whiteKnights; break;
                case BISHOP: promoBoard = &pos.whiteBishops; break;
                case ROOK:   promoBoard = &pos.whiteRooks; break;
                case QUEEN:  promoBoard = &pos.whiteQueens; break;
                default: promoBoard = &pos.whiteQueens; break;
            }
        } else {
            switch (move.promotion) {
                case KNIGHT: promoBoard = &pos.blackKnights; break;
                case BISHOP: promoBoard = &pos.blackBishops; break;
                case ROOK:   promoBoard = &pos.blackRooks; break;
                case QUEEN:  promoBoard = &pos.blackQueens; break;
                default: promoBoard = &pos.blackQueens; break;
            }
        }
        if (promoBoard)
            *promoBoard |= toMask;
    } else {
        *pieceBoard |= toMask;
    }

    // Castling: move the rook (same side as the king that just moved)
    if (move.isCastling) {
        uint64_t* rookBoard = pos.whiteToMove ? &pos.whiteRooks : &pos.blackRooks;
        int rookFrom, rookTo;
        if (move.to == 6) { rookFrom = 7; rookTo = 5; }   // white kingside: h1 -> f1
        else if (move.to == 2) { rookFrom = 0; rookTo = 3; }  // white queenside: a1 -> d1
        else if (move.to == 62) { rookFrom = 63; rookTo = 61; } // black kingside: h8 -> f8
        else { rookFrom = 56; rookTo = 59; }             // black queenside: a8 -> d8
        *rookBoard &= ~(1ULL << rookFrom);
        *rookBoard |= (1ULL << rookTo);
    }

    // Revoke castling rights when king or rook moves
    if (pos.whiteToMove) {  // we haven't switched yet, so the side that just moved is still "whiteToMove"
        if (move.piece == KING) { pos.whiteCanCastleK = false; pos.whiteCanCastleQ = false; }
        else if (move.piece == ROOK) {
            if (move.from == 7) pos.whiteCanCastleK = false;
            if (move.from == 0) pos.whiteCanCastleQ = false;
        }
        if (move.captured == ROOK) {
            if (move.to == 63) pos.blackCanCastleK = false;
            if (move.to == 56) pos.blackCanCastleQ = false;
        }
    } else {
        if (move.piece == KING) { pos.blackCanCastleK = false; pos.blackCanCastleQ = false; }
        else if (move.piece == ROOK) {
            if (move.from == 63) pos.blackCanCastleK = false;
            if (move.from == 56) pos.blackCanCastleQ = false;
        }
        if (move.captured == ROOK) {
            if (move.to == 7) pos.whiteCanCastleK = false;
            if (move.to == 0) pos.whiteCanCastleQ = false;
        }
    }

    pos.epSquare = newEpSquare;
    // Switch turn
    pos.whiteToMove = !pos.whiteToMove;
}

void undoMove(Position &pos, const State &previous) {
    pos = previous.position;
}