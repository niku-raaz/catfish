#pragma once

#include "catfish/move.hpp"
#include "catfish/types.hpp"
#include "catfish/zobrist.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace catfish {

struct UndoState {
    Move move{};
    PieceType captured_piece{PieceType::None};
    Square captured_square{kNoSquare};
    std::uint8_t castling_rights{NoCastling};
    Square en_passant_square{kNoSquare};
    int halfmove_clock{0};
    int fullmove_number{1};
    ZobristKey position_key{0};
};

class Board {
public:
    Board();

    static Board start_position();
    void clear();

    Bitboard pieces(Color color, PieceType piece) const;
    Bitboard occupancy(Color color) const;
    Bitboard occupancy_all() const;
    Color side_to_move() const;
    std::uint8_t castling_rights() const;
    Square en_passant_square() const;
    int halfmove_clock() const;
    int fullmove_number() const;
    ZobristKey position_key() const;

    void set_side_to_move(Color color);
    void set_castling_rights(std::uint8_t rights);
    void set_en_passant_square(Square square);
    void set_halfmove_clock(int clock);
    void set_fullmove_number(int number);

    void set_piece(Square square, Color color, PieceType piece);
    void remove_piece(Square square);
    std::optional<Piece> piece_at(Square square) const;
    Square king_square(Color color) const;

    UndoState make_move(const Move& move);
    void unmake_move(const UndoState& undo);

    bool is_square_attacked(Square square, Color by_color) const;
    bool in_check(Color color) const;
    std::string debug_string() const;

private:
    std::array<std::array<Bitboard, 6>, 2> pieces_{};
    Color side_to_move_{Color::White};
    std::uint8_t castling_rights_{NoCastling};
    Square en_passant_square_{kNoSquare};
    int halfmove_clock_{0};
    int fullmove_number_{1};
    ZobristKey position_key_{0};

    void clear_square_from_piece_sets(Square square);
    void put_piece(Square square, Color color, PieceType piece);
    void clear_piece(Square square, Color color, PieceType piece);
    ZobristKey en_passant_hash() const;
    void update_castling_rights_for_move(const Move& move, Square captured_square);
};

} // namespace catfish
