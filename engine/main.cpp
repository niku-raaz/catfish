#include "board.h"
#include "movegen.h"
#include "search.h"
#include <vector>
#include <iostream>

int main() {

    initKnightAttacks();
    initKingAttacks();

    Position pos;
    setStartPosition(pos);

    

    
    for(int i = 1; i <= 5; i++) {
        uint64_t nodes = perft(pos, i);
        std::cout << "Perft(" << i << "): " << nodes << std::endl;
    }

    return 0;
}