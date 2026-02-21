#include "board.h"
#include "movegen.h"
#include "search.h"
#include "eval.h"
#include <vector>
#include <iostream>

void testPreft(Position &pos){
    for (int i = 1; i <= 5; i++) {
        uint64_t nodes = perft(pos, i);
        std::cout << "Perft(" << i << "): " << nodes << std::endl;
    }

}

int main() {

    initKnightAttacks();
    initKingAttacks();

    Position pos;
    setStartPosition(pos);

    // Phase 1: Static evaluation verification
    std::cout << "Eval(start position): " << eval(pos) << " (expect 0)" << std::endl;

    // White up a queen -> positive
    pos.whiteQueens |= (1ULL << 32);  // add queen on e4
    std::cout << "Eval(white +Q on e4): " << eval(pos) << " (expect > 0, e.g. 900)" << std::endl;
    pos.whiteQueens &= ~(1ULL << 32);  // restore

    // Black up a rook -> negative
    setStartPosition(pos);
    pos.blackRooks |= (1ULL << 28);   // add black rook on e5
    std::cout << "Eval(black +R on e5): " << eval(pos) << " (expect < 0, e.g. -500)" << std::endl;

    setStartPosition(pos);
    std::cout << std::endl;

    // Phase 2: Minimax verification
    std::cout << "Minimax(depth 0): " << minimax(pos, 0) << " (expect 0, same as eval)" << std::endl;
    std::cout << "Minimax(depth 1): " << minimax(pos, 1) << " (max of 20 evals after 1 move)" << std::endl;
    std::cout << "Minimax(depth 2): " << minimax(pos, 2) << std::endl;

    std::cout << std::endl;
    //testPreft(pos);

    return 0;
}