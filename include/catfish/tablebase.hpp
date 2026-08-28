#pragma once

#include "catfish/board.hpp"
#include "catfish/move.hpp"

#include <iosfwd>
#include <optional>
#include <string>

namespace catfish {

enum class TablebaseWdl {
    Loss = -2,
    BlessedLoss = -1,
    Draw = 0,
    CursedWin = 1,
    Win = 2
};

struct TablebaseRootResult {
    Move move{};
    TablebaseWdl wdl{TablebaseWdl::Draw};
    unsigned distance_to_zero{0};
};

class Tablebase {
public:
    Tablebase();
    ~Tablebase();

    Tablebase(const Tablebase&) = delete;
    Tablebase& operator=(const Tablebase&) = delete;

    bool initialize(const std::string& path, std::ostream& diagnostics);
    void clear();
    bool available() const;
    unsigned largest_piece_count() const;
    std::optional<TablebaseRootResult> probe_root(Board& board) const;

private:
    bool backend_initialized_{false};
    bool initialized_{false};
    unsigned largest_piece_count_{0};
};

} // namespace catfish
