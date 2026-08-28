#pragma once

#include "catfish/board.hpp"
#include "catfish/move.hpp"
#include "catfish/zobrist.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace catfish {

struct SearchResult {
    std::optional<Move> best_move{};
    int score{0};
    std::uint64_t nodes{0};
    std::uint64_t quiescence_nodes{0};
    std::uint64_t transposition_hits{0};
    std::uint64_t tablebase_hits{0};
    int depth{0};
    int selective_depth{0};
    std::vector<Move> principal_variation{};
    bool book_hit{false};
    bool tablebase_hit{false};
    std::string opening_name{};
};

class SearchEngine {
public:
    explicit SearchEngine(std::size_t hash_size_mb = 32);
    ~SearchEngine();

    SearchEngine(SearchEngine&&) noexcept;
    SearchEngine& operator=(SearchEngine&&) noexcept;
    SearchEngine(const SearchEngine&) = delete;
    SearchEngine& operator=(const SearchEngine&) = delete;

    void clear();
    void set_hash_size(std::size_t hash_size_mb);
    SearchResult search(
        Board& board,
        int depth,
        const std::vector<ZobristKey>& game_history = {}
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

SearchResult search_best_move(
    Board& board,
    int depth,
    const std::vector<ZobristKey>& game_history = {}
);

} // namespace catfish
