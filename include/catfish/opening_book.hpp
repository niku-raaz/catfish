#pragma once

#include "catfish/board.hpp"
#include "catfish/move.hpp"

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>

namespace catfish {

struct BookChoice {
    Move move{};
    int weight{0};
    std::string opening{};
};

class OpeningBook {
public:
    OpeningBook();

    void reset_to_builtin();
    bool load_file(const std::string& path, std::ostream& diagnostics);
    std::optional<BookChoice> probe(Board& board) const;
    std::size_t position_count() const;

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

} // namespace catfish
