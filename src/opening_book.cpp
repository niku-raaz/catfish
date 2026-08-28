#include "catfish/opening_book.hpp"

#include "catfish/movegen.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace catfish {

namespace {

struct BookLine {
    int weight;
    const char* name;
    const char* moves;
};

constexpr BookLine kBuiltinLines[] = {
    {34, "Ruy Lopez", "e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7"},
    {28, "Italian Game", "e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 c2c3 g8f6 d2d3 d7d6"},
    {40, "Sicilian Defense: Najdorf", "e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6"},
    {24, "French Defense", "e2e4 e7e6 d2d4 d7d5 b1c3 g8f6 e4e5 f6d7 f2f4 c7c5"},
    {24, "Caro-Kann Defense", "e2e4 c7c6 d2d4 d7d5 b1c3 d5e4 c3e4 c8f5"},
    {38, "Queen's Gambit Declined", "d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 c1g5 f8e7"},
    {28, "Slav Defense", "d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 b1c3 d5c4"},
    {36, "Nimzo-Indian Defense", "d2d4 g8f6 c2c4 e7e6 b1c3 f8b4"},
    {32, "King's Indian Defense", "d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6"},
    {28, "English Opening", "c2c4 e7e5 b1c3 g8f6 g2g3 d7d5 c4d5 f6d5 f1g2"},
    {24, "Réti Opening", "g1f3 d7d5 g2g3 g8f6 f1g2 g7g6 e1g1 f8g7"},
};

std::vector<std::string> split_moves(std::string_view text) {
    std::istringstream input{std::string(text)};
    std::vector<std::string> moves;
    std::string move;
    while (input >> move) {
        moves.push_back(move);
    }
    return moves;
}

bool same_move(const Move& left, const Move& right) {
    return left.from == right.from
        && left.to == right.to
        && left.promotion == right.promotion;
}

std::optional<Move> legal_move_from_text(Board& board, std::string_view text) {
    auto legal = generate_legal_moves(board);
    const auto found = std::find_if(legal.begin(), legal.end(), [&](const Move& move) {
        return move_to_string(move) == text;
    });
    return found == legal.end() ? std::nullopt : std::optional<Move>(*found);
}

} // namespace

struct OpeningBook::Impl {
    std::unordered_map<ZobristKey, std::vector<BookChoice>> positions{};

    bool add_line(
        int weight,
        const std::string& name,
        std::string_view moves,
        std::ostream* diagnostics
    ) {
        Board board = Board::start_position();
        for (const std::string& text : split_moves(moves)) {
            const auto move = legal_move_from_text(board, text);
            if (!move.has_value()) {
                if (diagnostics != nullptr) {
                    *diagnostics << "invalid opening line move " << text
                                 << " in " << name << '\n';
                }
                return false;
            }

            auto& choices = positions[board.position_key()];
            const auto existing = std::find_if(
                choices.begin(),
                choices.end(),
                [&](const BookChoice& choice) {
                    return same_move(choice.move, *move);
                }
            );
            if (existing == choices.end()) {
                choices.push_back(BookChoice{*move, weight, name});
            } else {
                existing->weight += weight;
            }
            board.make_move(*move);
        }
        return true;
    }
};

OpeningBook::OpeningBook()
    : impl_(std::make_shared<Impl>()) {
    reset_to_builtin();
}

void OpeningBook::reset_to_builtin() {
    auto replacement = std::make_shared<Impl>();
    for (const BookLine& line : kBuiltinLines) {
        replacement->add_line(line.weight, line.name, line.moves, nullptr);
    }
    impl_ = std::move(replacement);
}

bool OpeningBook::load_file(const std::string& path, std::ostream& diagnostics) {
    std::ifstream input(path);
    if (!input) {
        diagnostics << "unable to open opening book: " << path << '\n';
        return false;
    }

    auto replacement = std::make_shared<Impl>();
    std::string line;
    int line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const std::size_t first_tab = line.find('\t');
        const std::size_t second_tab = first_tab == std::string::npos
            ? std::string::npos
            : line.find('\t', first_tab + 1);
        if (first_tab == std::string::npos || second_tab == std::string::npos) {
            diagnostics << "invalid opening book line " << line_number
                        << ": expected weight, name, and UCI moves\n";
            return false;
        }
        int weight = 0;
        const std::string_view weight_text(line.data(), first_tab);
        const auto parsed = std::from_chars(
            weight_text.data(),
            weight_text.data() + weight_text.size(),
            weight
        );
        if (parsed.ec != std::errc{} || parsed.ptr != weight_text.data() + weight_text.size()
            || weight <= 0) {
            diagnostics << "invalid opening book weight on line " << line_number << '\n';
            return false;
        }
        const std::string name = line.substr(first_tab + 1, second_tab - first_tab - 1);
        if (!replacement->add_line(weight, name, line.substr(second_tab + 1), &diagnostics)) {
            diagnostics << "opening book load aborted on line " << line_number << '\n';
            return false;
        }
    }
    if (replacement->positions.empty()) {
        diagnostics << "opening book contains no usable positions\n";
        return false;
    }
    impl_ = std::move(replacement);
    return true;
}

std::optional<BookChoice> OpeningBook::probe(Board& board) const {
    const auto found = impl_->positions.find(board.position_key());
    if (found == impl_->positions.end()) {
        return std::nullopt;
    }

    auto legal = generate_legal_moves(board);
    std::optional<BookChoice> best;
    for (const BookChoice& choice : found->second) {
        const auto legal_move = std::find_if(legal.begin(), legal.end(), [&](const Move& move) {
            return same_move(move, choice.move);
        });
        if (legal_move != legal.end() && (!best.has_value() || choice.weight > best->weight)) {
            best = choice;
            best->move = *legal_move;
        }
    }
    return best;
}

std::size_t OpeningBook::position_count() const {
    return impl_->positions.size();
}

} // namespace catfish
