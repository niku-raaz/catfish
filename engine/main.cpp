#include "board.h"
#include "movegen.h"
#include <vector>
#include <iostream>

int main() {
    initKnightAttacks();

    Position pos;
    setStartPosition(pos);

    std::vector<Move> moves;
    //generatePawnMoves(pos, moves);
    generateKnightMoves(pos, WHITE,moves);
    generateKnightMoves(pos, BLACK,moves);
    printBoard(pos);

    std::cout<<"Total Knight move "<< moves.size()<<" ";

    

    return 0;
}