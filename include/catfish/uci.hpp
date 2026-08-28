#pragma once

#include "catfish/board.hpp"
#include "catfish/opening_book.hpp"
#include "catfish/search.hpp"
#include "catfish/tablebase.hpp"

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace catfish {

std::optional<Move> move_from_uci(Board& board, std::string_view text);
bool apply_uci_move(Board& board, std::string_view text);

class UciSession {
public:
    UciSession();

    bool handle_command(
        const std::string& command,
        std::ostream& output,
        std::ostream& diagnostics
    );

    const Board& board() const;

private:
    Board board_;
    SearchEngine search_engine_;
    OpeningBook opening_book_;
    Tablebase tablebase_;
    std::vector<ZobristKey> position_history_;
    bool own_book_{true};

    bool set_position(const std::string& command, std::ostream& diagnostics);
    void set_option(const std::string& command, std::ostream& diagnostics);
    void search(const std::string& command, std::ostream& output, std::ostream& diagnostics);
};

void run_uci(std::istream& input, std::ostream& output, std::ostream& diagnostics);

} // namespace catfish
