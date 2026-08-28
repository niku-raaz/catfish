#include "catfish/fen.hpp"
#include "catfish/perft.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::string fen_from_args(int argc, char** argv, int start_index) {
    if (argc <= start_index) {
        return catfish::kStartFen;
    }

    std::string fen;
    for (int i = start_index; i < argc; ++i) {
        if (!fen.empty()) {
            fen += ' ';
        }
        fen += argv[i];
    }
    return fen;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: catfish_perft <depth> [fen]\n";
        return 1;
    }

    const int depth = std::atoi(argv[1]);
    auto board = catfish::board_from_fen(fen_from_args(argc, argv, 2));
    std::uint64_t total = 0;

    for (const auto& entry : catfish::perft_divide(board, depth)) {
        std::cout << catfish::move_to_string(entry.move) << ": " << entry.nodes << '\n';
        total += entry.nodes;
    }

    std::cout << "Total: " << total << '\n';
    return 0;
}

