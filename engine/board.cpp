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