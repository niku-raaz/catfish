#include "catfish/eval.hpp"
#include "catfish/fen.hpp"
#include "catfish/movegen.hpp"
#include "catfish/perft.hpp"
#include "catfish/search.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void print_usage() {
    std::cout
        << "Catfish chess engine\n"
        << "Usage:\n"
        << "  catfish board [fen]\n"
        << "  catfish eval [fen]\n"
        << "  catfish evaltrace [fen]\n"
        << "  catfish perft <depth> [fen]\n"
        << "  catfish search <depth> [fen]\n"
        << "  catfish bench [depth]\n";
}

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
        print_usage();
        return 0;
    }

    const std::string command = argv[1];

    try {
        if (command == "board") {
            auto board = catfish::board_from_fen(fen_from_args(argc, argv, 2));
            std::cout << board.debug_string();
            std::cout << catfish::board_to_fen(board) << '\n';
            return 0;
        }

        if (command == "eval") {
            auto board = catfish::board_from_fen(fen_from_args(argc, argv, 2));
            std::cout << catfish::evaluate(board) << '\n';
            return 0;
        }

        if (command == "evaltrace") {
            const auto board = catfish::board_from_fen(fen_from_args(argc, argv, 2));
            const auto trace = catfish::evaluate_with_breakdown(board);
            std::cout << "material " << trace.material << '\n'
                      << "piece_square " << trace.piece_square << '\n'
                      << "pawn_structure " << trace.pawn_structure << '\n'
                      << "mobility " << trace.mobility << '\n'
                      << "king_safety " << trace.king_safety << '\n'
                      << "positional " << trace.positional << '\n'
                      << "phase " << trace.phase << "/24\n"
                      << "total " << trace.score << '\n';
            return 0;
        }

        if (command == "perft") {
            if (argc < 3) {
                print_usage();
                return 1;
            }
            const int depth = std::atoi(argv[2]);
            auto board = catfish::board_from_fen(fen_from_args(argc, argv, 3));
            std::cout << catfish::perft(board, depth) << '\n';
            return 0;
        }

        if (command == "search") {
            if (argc < 3) {
                print_usage();
                return 1;
            }
            const int depth = std::atoi(argv[2]);
            auto board = catfish::board_from_fen(fen_from_args(argc, argv, 3));
            const auto result = catfish::search_best_move(board, depth);
            if (result.best_move.has_value()) {
                std::cout << "bestmove " << catfish::move_to_string(*result.best_move)
                          << " score " << result.score
                          << " nodes " << result.nodes << '\n';
            } else {
                std::cout << "bestmove none score " << result.score << '\n';
            }
            return 0;
        }

        if (command == "bench") {
            const int depth = argc >= 3 ? std::atoi(argv[2]) : 4;
            if (depth < 1 || depth > 12) {
                throw std::invalid_argument("bench depth must be between 1 and 12");
            }
            constexpr std::array<const char*, 5> positions{{
                catfish::kStartFen,
                "r1bq1rk1/pp2bppp/2n1pn2/2pp4/3P4/2P1PN2/PP1NBPPP/R1BQ1RK1 w - - 4 8",
                "2r2rk1/1bqnbppp/p2ppn2/1p6/3NP3/P1N1B3/1P2BPPP/2RQ1RK1 w - - 2 13",
                "8/5pk1/6p1/3p4/3P1P2/5KP1/8/8 w - - 0 35",
                "8/2p5/3p1k2/1p1Pp3/p3P2P/P4KP1/1P6/8 w - - 0 42",
            }};
            catfish::SearchEngine engine(32);
            std::uint64_t nodes = 0;
            std::uint64_t qnodes = 0;
            std::uint64_t hash_hits = 0;
            const auto started = std::chrono::steady_clock::now();
            for (const char* fen : positions) {
                auto board = catfish::board_from_fen(fen);
                const auto result = engine.search(board, depth);
                nodes += result.nodes;
                qnodes += result.quiescence_nodes;
                hash_hits += result.transposition_hits;
            }
            const auto elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - started
            ).count();
            const auto nodes_per_second = elapsed > 0.0
                ? static_cast<std::uint64_t>(static_cast<double>(nodes) / elapsed)
                : 0;
            std::cout << "bench depth " << depth
                      << " positions " << positions.size()
                      << " nodes " << nodes
                      << " qnodes " << qnodes
                      << " hashhits " << hash_hits
                      << " time_ms " << static_cast<std::uint64_t>(elapsed * 1000.0)
                      << " nps " << nodes_per_second << '\n';
            return 0;
        }
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    print_usage();
    return 1;
}
