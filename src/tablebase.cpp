#include "catfish/tablebase.hpp"

#include "catfish/bitboard.hpp"
#include "catfish/movegen.hpp"

#include <algorithm>
#include <ostream>

#ifdef CATFISH_HAS_FATHOM
#include <tbprobe.h>
#endif

namespace catfish {

namespace {

#ifdef CATFISH_HAS_FATHOM
bool same_tablebase_move(
    const Move& move,
    Square from,
    Square to,
    PieceType promotion
) {
    return move.from == from
        && move.to == to
        && move.promotion == promotion;
}

PieceType promotion_from_fathom(unsigned promotion) {
    switch (promotion) {
        case TB_PROMOTES_QUEEN: return PieceType::Queen;
        case TB_PROMOTES_ROOK: return PieceType::Rook;
        case TB_PROMOTES_BISHOP: return PieceType::Bishop;
        case TB_PROMOTES_KNIGHT: return PieceType::Knight;
        default: return PieceType::None;
    }
}

TablebaseWdl wdl_from_fathom(unsigned wdl) {
    switch (wdl) {
        case TB_LOSS: return TablebaseWdl::Loss;
        case TB_BLESSED_LOSS: return TablebaseWdl::BlessedLoss;
        case TB_CURSED_WIN: return TablebaseWdl::CursedWin;
        case TB_WIN: return TablebaseWdl::Win;
        default: return TablebaseWdl::Draw;
    }
}
#endif

} // namespace

Tablebase::Tablebase() = default;

Tablebase::~Tablebase() {
    clear();
}

bool Tablebase::initialize(const std::string& path, std::ostream& diagnostics) {
    clear();
#ifdef CATFISH_HAS_FATHOM
    if (path.empty()) {
        diagnostics << "SyzygyPath cannot be empty\n";
        return false;
    }
    if (!tb_init(path.c_str())) {
        diagnostics << "Fathom could not initialize SyzygyPath: " << path << '\n';
        return false;
    }
    backend_initialized_ = true;
    largest_piece_count_ = TB_LARGEST;
    initialized_ = largest_piece_count_ > 0;
    if (!initialized_) {
        diagnostics << "no Syzygy tablebase files found at: " << path << '\n';
    }
    return initialized_;
#else
    (void)path;
    diagnostics << "Catfish was built without Fathom; configure with "
                << "-DCATFISH_ENABLE_SYZYGY=ON and -DCATFISH_FATHOM_ROOT=<path>\n";
    return false;
#endif
}

void Tablebase::clear() {
#ifdef CATFISH_HAS_FATHOM
    if (backend_initialized_) {
        tb_free();
    }
#endif
    backend_initialized_ = false;
    initialized_ = false;
    largest_piece_count_ = 0;
}

bool Tablebase::available() const {
    return initialized_;
}

unsigned Tablebase::largest_piece_count() const {
    return largest_piece_count_;
}

std::optional<TablebaseRootResult> Tablebase::probe_root(Board& board) const {
#ifdef CATFISH_HAS_FATHOM
    if (!initialized_
        || popcount(board.occupancy_all()) > static_cast<int>(largest_piece_count_)
        || board.castling_rights() != NoCastling) {
        return std::nullopt;
    }

    const Bitboard white = board.occupancy(Color::White);
    const Bitboard black = board.occupancy(Color::Black);
    const Bitboard kings = board.pieces(Color::White, PieceType::King)
        | board.pieces(Color::Black, PieceType::King);
    const Bitboard queens = board.pieces(Color::White, PieceType::Queen)
        | board.pieces(Color::Black, PieceType::Queen);
    const Bitboard rooks = board.pieces(Color::White, PieceType::Rook)
        | board.pieces(Color::Black, PieceType::Rook);
    const Bitboard bishops = board.pieces(Color::White, PieceType::Bishop)
        | board.pieces(Color::Black, PieceType::Bishop);
    const Bitboard knights = board.pieces(Color::White, PieceType::Knight)
        | board.pieces(Color::Black, PieceType::Knight);
    const Bitboard pawns = board.pieces(Color::White, PieceType::Pawn)
        | board.pieces(Color::Black, PieceType::Pawn);
    const unsigned ep = board.en_passant_square() == kNoSquare
        ? 0U
        : static_cast<unsigned>(board.en_passant_square());

    const unsigned result = tb_probe_root(
        white,
        black,
        kings,
        queens,
        rooks,
        bishops,
        knights,
        pawns,
        static_cast<unsigned>(board.halfmove_clock()),
        static_cast<unsigned>(board.castling_rights()),
        ep,
        board.side_to_move() == Color::White,
        nullptr
    );
    if (result == TB_RESULT_FAILED
        || result == TB_RESULT_CHECKMATE
        || result == TB_RESULT_STALEMATE) {
        return std::nullopt;
    }

    const Square from = static_cast<Square>(TB_GET_FROM(result));
    const Square to = static_cast<Square>(TB_GET_TO(result));
    const PieceType promotion = promotion_from_fathom(TB_GET_PROMOTES(result));
    auto legal = generate_legal_moves(board);
    const auto move = std::find_if(legal.begin(), legal.end(), [&](const Move& candidate) {
        return same_tablebase_move(candidate, from, to, promotion);
    });
    if (move == legal.end()) {
        return std::nullopt;
    }
    return TablebaseRootResult{
        *move,
        wdl_from_fathom(TB_GET_WDL(result)),
        TB_GET_DTZ(result)
    };
#else
    (void)board;
    return std::nullopt;
#endif
}

} // namespace catfish
