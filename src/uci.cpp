#include "catfish/uci.hpp"

#include "catfish/eval.hpp"
#include "catfish/fen.hpp"
#include "catfish/movegen.hpp"
#include "catfish/search.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace catfish {

namespace {

constexpr int kMaximumUciDepth = 20;
constexpr int kMateThreshold = 90000;
constexpr int kMateScore = 100000;

std::vector<std::string> words(const std::string& text) {
    std::istringstream input(text);
    std::vector<std::string> result;
    std::string word;
    while (input >> word) {
        result.push_back(word);
    }
    return result;
}

std::optional<int> parse_integer(const std::string& text) {
    int value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

void write_search_info(std::ostream& output, const SearchResult& result) {
    output << "info depth " << result.depth
           << " seldepth " << result.selective_depth
           << " score ";
    if (std::abs(result.score) >= kMateThreshold) {
        const int plies = kMateScore - std::abs(result.score);
        const int moves = std::max(1, (plies + 1) / 2);
        output << "mate " << (result.score > 0 ? moves : -moves);
    } else {
        output << "cp " << result.score;
    }
    output << " nodes " << result.nodes
           << " qnodes " << result.quiescence_nodes
           << " tbhits " << result.tablebase_hits
           << " hashhits " << result.transposition_hits;
    if (!result.principal_variation.empty()) {
        output << " pv";
        for (const Move& move : result.principal_variation) {
            output << ' ' << move_to_string(move);
        }
    } else if (result.best_move.has_value()) {
        output << " pv " << move_to_string(*result.best_move);
    }
    output << '\n';
}

} // namespace

std::optional<Move> move_from_uci(Board& board, std::string_view text) {
    if (text.size() != 4 && text.size() != 5) {
        return std::nullopt;
    }

    auto legal_moves = generate_legal_moves(board);
    const auto found = std::find_if(legal_moves.begin(), legal_moves.end(), [&](const Move& move) {
        return move_to_string(move) == text;
    });
    if (found == legal_moves.end()) {
        return std::nullopt;
    }
    return *found;
}

bool apply_uci_move(Board& board, std::string_view text) {
    const auto move = move_from_uci(board, text);
    if (!move.has_value()) {
        return false;
    }
    board.make_move(*move);
    return true;
}

UciSession::UciSession()
    : board_(Board::start_position()),
      search_engine_(32),
      position_history_{board_.position_key()} {}

const Board& UciSession::board() const {
    return board_;
}

bool UciSession::set_position(const std::string& command, std::ostream& diagnostics) {
    const auto tokens = words(command);
    if (tokens.size() < 2) {
        diagnostics << "position command requires startpos or fen\n";
        return false;
    }

    Board candidate;
    std::vector<ZobristKey> candidate_history;
    std::size_t index = 0;

    try {
        if (tokens[1] == "startpos") {
            candidate = Board::start_position();
            index = 2;
        } else if (tokens[1] == "fen") {
            if (tokens.size() < 8) {
                diagnostics << "position fen requires six FEN fields\n";
                return false;
            }
            std::string fen;
            for (std::size_t i = 2; i < 8; ++i) {
                if (!fen.empty()) {
                    fen += ' ';
                }
                fen += tokens[i];
            }
            candidate = board_from_fen(fen);
            index = 8;
        } else {
            diagnostics << "unsupported position source: " << tokens[1] << '\n';
            return false;
        }
    } catch (const std::exception& error) {
        diagnostics << "invalid position: " << error.what() << '\n';
        return false;
    }
    candidate_history.push_back(candidate.position_key());

    if (index < tokens.size()) {
        if (tokens[index] != "moves") {
            diagnostics << "expected moves after position\n";
            return false;
        }
        ++index;
    }

    for (; index < tokens.size(); ++index) {
        if (!apply_uci_move(candidate, tokens[index])) {
            diagnostics << "illegal move in position command: " << tokens[index] << '\n';
            return false;
        }
        candidate_history.push_back(candidate.position_key());
    }

    board_ = candidate;
    position_history_ = std::move(candidate_history);
    return true;
}

void UciSession::set_option(const std::string& command, std::ostream& diagnostics) {
    const auto tokens = words(command);
    if (tokens.size() < 3 || tokens[1] != "name") {
        diagnostics << "setoption requires a name\n";
        return;
    }

    const auto value_token = std::find(tokens.begin() + 2, tokens.end(), "value");
    const std::size_t name_end = static_cast<std::size_t>(value_token - tokens.begin());
    std::string name;
    for (std::size_t index = 2; index < name_end; ++index) {
        if (!name.empty()) {
            name += ' ';
        }
        name += tokens[index];
    }
    std::string value;
    if (value_token != tokens.end()) {
        for (auto iterator = value_token + 1; iterator != tokens.end(); ++iterator) {
            if (!value.empty()) {
                value += ' ';
            }
            value += *iterator;
        }
    }

    if (name == "Hash") {
        const auto parsed = parse_integer(value);
        if (!parsed.has_value() || *parsed < 1 || *parsed > 1024) {
            diagnostics << "Hash must be between 1 and 1024 MB\n";
            return;
        }
        search_engine_.set_hash_size(static_cast<std::size_t>(*parsed));
    } else if (name == "Clear Hash") {
        search_engine_.clear();
    } else if (name == "OwnBook") {
        if (value == "true") {
            own_book_ = true;
        } else if (value == "false") {
            own_book_ = false;
        } else {
            diagnostics << "OwnBook must be true or false\n";
        }
    } else if (name == "BookFile") {
        if (value.empty() || value == "<builtin>") {
            opening_book_.reset_to_builtin();
        } else {
            opening_book_.load_file(value, diagnostics);
        }
    } else if (name == "SyzygyPath") {
        if (value.empty() || value == "<empty>") {
            tablebase_.clear();
        } else {
            tablebase_.initialize(value, diagnostics);
        }
    } else {
        diagnostics << "unsupported UCI option: " << name << '\n';
    }
}

void UciSession::search(
    const std::string& command,
    std::ostream& output,
    std::ostream& diagnostics
) {
    const auto tokens = words(command);
    int depth = 3;

    for (std::size_t index = 1; index < tokens.size(); ++index) {
        if (tokens[index] != "depth") {
            continue;
        }
        if (index + 1 >= tokens.size()) {
            diagnostics << "go depth requires a value\n";
            return;
        }
        const auto parsed = parse_integer(tokens[index + 1]);
        if (!parsed.has_value() || *parsed < 1 || *parsed > kMaximumUciDepth) {
            diagnostics << "search depth must be between 1 and " << kMaximumUciDepth << '\n';
            return;
        }
        depth = *parsed;
        break;
    }

    auto position = board_;
    SearchResult result;
    const auto tablebase_result = tablebase_.probe_root(position);
    if (tablebase_result.has_value()) {
        result.best_move = tablebase_result->move;
        result.principal_variation.push_back(tablebase_result->move);
        result.tablebase_hit = true;
        result.tablebase_hits = 1;
        switch (tablebase_result->wdl) {
            case TablebaseWdl::Win: result.score = 20000; break;
            case TablebaseWdl::Loss: result.score = -20000; break;
            case TablebaseWdl::BlessedLoss:
            case TablebaseWdl::Draw:
            case TablebaseWdl::CursedWin:
                result.score = 0;
                break;
        }
        output << "info string syzygy dtz "
               << tablebase_result->distance_to_zero << '\n';
    } else if (own_book_) {
        const auto choice = opening_book_.probe(position);
        if (choice.has_value()) {
            result.best_move = choice->move;
            result.principal_variation.push_back(choice->move);
            const int white_score = evaluate(position);
            result.score = position.side_to_move() == Color::White ? white_score : -white_score;
            result.book_hit = true;
            result.opening_name = choice->opening;
        }
    }
    if (!result.book_hit && !result.tablebase_hit) {
        result = search_engine_.search(position, depth, position_history_);
    } else if (result.book_hit) {
        output << "info string book " << result.opening_name << '\n';
    }
    write_search_info(output, result);
    output << "bestmove "
           << (result.best_move.has_value() ? move_to_string(*result.best_move) : "0000")
           << '\n';
    output.flush();
}

bool UciSession::handle_command(
    const std::string& command,
    std::ostream& output,
    std::ostream& diagnostics
) {
    const auto tokens = words(command);
    if (tokens.empty()) {
        return true;
    }

    const std::string& name = tokens.front();
    if (name == "uci") {
        output << "id name Catfish\n";
        output << "id author Catfish contributors\n";
        output << "option name Hash type spin default 32 min 1 max 1024\n";
        output << "option name Clear Hash type button\n";
        output << "option name OwnBook type check default true\n";
        output << "option name BookFile type string default <builtin>\n";
        output << "option name SyzygyPath type string default <empty>\n";
        output << "uciok\n";
        output.flush();
    } else if (name == "isready") {
        output << "readyok\n";
        output.flush();
    } else if (name == "ucinewgame") {
        board_ = Board::start_position();
        position_history_ = {board_.position_key()};
        search_engine_.clear();
    } else if (name == "setoption") {
        set_option(command, diagnostics);
    } else if (name == "position") {
        set_position(command, diagnostics);
    } else if (name == "go") {
        search(command, output, diagnostics);
    } else if (name == "stop") {
        // Search is currently synchronous. An idle stop is intentionally safe.
    } else if (name == "quit") {
        return false;
    } else {
        diagnostics << "unsupported UCI command: " << name << '\n';
    }
    return true;
}

void run_uci(std::istream& input, std::ostream& output, std::ostream& diagnostics) {
    UciSession session;
    std::string command;
    while (std::getline(input, command)) {
        if (!session.handle_command(command, output, diagnostics)) {
            return;
        }
    }
}

} // namespace catfish
