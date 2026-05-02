#include "chess/core/board.h"

#include <cassert>

#include "chess/core/fen.h"

namespace chess {

namespace {

constexpr int color_index(Color color) {
    return static_cast<int>(color);
}

constexpr int type_index(PieceType type) {
    return static_cast<int>(type);
}

bool square_has_piece(const Board& board, Square square, Color color, PieceType type) {
    return is_valid_square(square) && board.piece_at(square) == make_piece(color, type);
}

}  // namespace

Board::Board() {
    clear();
}

Board Board::start_position() {
    return board_from_fen(kStartFen);
}

Piece Board::piece_at(Square square) const {
    assert(is_valid_square(square));
    return squares_[square];
}

Color Board::side_to_move() const {
    return side_to_move_;
}

std::uint8_t Board::castling_rights() const {
    return castling_rights_;
}

Square Board::en_passant_square() const {
    return en_passant_square_;
}

int Board::halfmove_clock() const {
    return halfmove_clock_;
}

int Board::fullmove_number() const {
    return fullmove_number_;
}

Bitboard Board::occupancy(Color color) const {
    return occupancy_[color_index(color)];
}

Bitboard Board::occupancy() const {
    return all_occupancy_;
}

Bitboard Board::pieces(Color color, PieceType type) const {
    return pieces_[color_index(color)][type_index(type)];
}

Square Board::king_square(Color color) const {
    const Bitboard king = pieces(color, PieceType::King);
    if (king == 0) {
        return kNoSquare;
    }
    return __builtin_ctzll(king);
}

bool Board::is_square_attacked(Square square, Color by_color) const {
    assert(is_valid_square(square));
    const int file = file_of(square);
    const int rank = rank_of(square);

    if (by_color == Color::White) {
        if (file > 0 && square_has_piece(*this, square - 9, Color::White, PieceType::Pawn)) {
            return true;
        }
        if (file < 7 && square_has_piece(*this, square - 7, Color::White, PieceType::Pawn)) {
            return true;
        }
    } else {
        if (file > 0 && square_has_piece(*this, square + 7, Color::Black, PieceType::Pawn)) {
            return true;
        }
        if (file < 7 && square_has_piece(*this, square + 9, Color::Black, PieceType::Pawn)) {
            return true;
        }
    }

    constexpr std::array<std::pair<int, int>, 8> knight_offsets{{
        {1, 2}, {2, 1}, {2, -1}, {1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2},
    }};
    for (const auto& [df, dr] : knight_offsets) {
        const int nf = file + df;
        const int nr = rank + dr;
        if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8
            && square_has_piece(*this, make_square(nf, nr), by_color, PieceType::Knight)) {
            return true;
        }
    }

    constexpr std::array<std::pair<int, int>, 8> king_offsets{{
        {1, 1}, {1, 0}, {1, -1}, {0, -1},
        {-1, -1}, {-1, 0}, {-1, 1}, {0, 1},
    }};
    for (const auto& [df, dr] : king_offsets) {
        const int nf = file + df;
        const int nr = rank + dr;
        if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8
            && square_has_piece(*this, make_square(nf, nr), by_color, PieceType::King)) {
            return true;
        }
    }

    constexpr std::array<std::pair<int, int>, 4> bishop_dirs{{
        {1, 1}, {1, -1}, {-1, -1}, {-1, 1},
    }};
    for (const auto& [df, dr] : bishop_dirs) {
        int nf = file + df;
        int nr = rank + dr;
        while (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
            const Piece piece = piece_at(make_square(nf, nr));
            if (piece != Piece::None) {
                if (color_of(piece) == by_color
                    && (type_of(piece) == PieceType::Bishop || type_of(piece) == PieceType::Queen)) {
                    return true;
                }
                break;
            }
            nf += df;
            nr += dr;
        }
    }

    constexpr std::array<std::pair<int, int>, 4> rook_dirs{{
        {1, 0}, {0, -1}, {-1, 0}, {0, 1},
    }};
    for (const auto& [df, dr] : rook_dirs) {
        int nf = file + df;
        int nr = rank + dr;
        while (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
            const Piece piece = piece_at(make_square(nf, nr));
            if (piece != Piece::None) {
                if (color_of(piece) == by_color
                    && (type_of(piece) == PieceType::Rook || type_of(piece) == PieceType::Queen)) {
                    return true;
                }
                break;
            }
            nf += df;
            nr += dr;
        }
    }

    return false;
}

bool Board::in_check(Color color) const {
    const Square king = king_square(color);
    return is_valid_square(king) && is_square_attacked(king, opposite(color));
}

void Board::clear() {
    squares_.fill(Piece::None);
    for (auto& color_pieces : pieces_) {
        color_pieces.fill(0);
    }
    occupancy_.fill(0);
    all_occupancy_ = 0;
    side_to_move_ = Color::White;
    castling_rights_ = 0;
    en_passant_square_ = kNoSquare;
    halfmove_clock_ = 0;
    fullmove_number_ = 1;
}

void Board::set_piece(Square square, Piece piece) {
    assert(is_valid_square(square));
    remove_piece(square);
    squares_[square] = piece;
    if (piece == Piece::None) {
        return;
    }

    const auto color = color_of(piece);
    const auto type = type_of(piece);
    const Bitboard bit = square_bb(square);
    pieces_[color_index(color)][type_index(type)] |= bit;
    occupancy_[color_index(color)] |= bit;
    all_occupancy_ |= bit;
}

void Board::remove_piece(Square square) {
    assert(is_valid_square(square));
    const Piece piece = squares_[square];
    if (piece == Piece::None) {
        return;
    }

    const auto color = color_of(piece);
    const auto type = type_of(piece);
    const Bitboard bit = square_bb(square);
    pieces_[color_index(color)][type_index(type)] &= ~bit;
    occupancy_[color_index(color)] &= ~bit;
    all_occupancy_ &= ~bit;
    squares_[square] = Piece::None;
}

void Board::set_side_to_move(Color color) {
    side_to_move_ = color;
}

void Board::set_castling_rights(std::uint8_t rights) {
    castling_rights_ = rights;
}

void Board::set_en_passant_square(Square square) {
    en_passant_square_ = square;
}

void Board::set_halfmove_clock(int value) {
    halfmove_clock_ = value;
}

void Board::set_fullmove_number(int value) {
    fullmove_number_ = value;
}

void Board::move_piece(Square from, Square to) {
    const Piece piece = piece_at(from);
    remove_piece(from);
    set_piece(to, piece);
}

void Board::update_castling_rights_for_move(Square from, Square to, Piece moved, Piece captured) {
    if (type_of(moved) == PieceType::King) {
        if (color_of(moved) == Color::White) {
            castling_rights_ &= static_cast<std::uint8_t>(~(WhiteKingSide | WhiteQueenSide));
        } else {
            castling_rights_ &= static_cast<std::uint8_t>(~(BlackKingSide | BlackQueenSide));
        }
    }

    if (type_of(moved) == PieceType::Rook) {
        if (from == make_square(0, 0)) {
            castling_rights_ &= static_cast<std::uint8_t>(~WhiteQueenSide);
        } else if (from == make_square(7, 0)) {
            castling_rights_ &= static_cast<std::uint8_t>(~WhiteKingSide);
        } else if (from == make_square(0, 7)) {
            castling_rights_ &= static_cast<std::uint8_t>(~BlackQueenSide);
        } else if (from == make_square(7, 7)) {
            castling_rights_ &= static_cast<std::uint8_t>(~BlackKingSide);
        }
    }

    if (type_of(captured) == PieceType::Rook) {
        if (to == make_square(0, 0)) {
            castling_rights_ &= static_cast<std::uint8_t>(~WhiteQueenSide);
        } else if (to == make_square(7, 0)) {
            castling_rights_ &= static_cast<std::uint8_t>(~WhiteKingSide);
        } else if (to == make_square(0, 7)) {
            castling_rights_ &= static_cast<std::uint8_t>(~BlackQueenSide);
        } else if (to == make_square(7, 7)) {
            castling_rights_ &= static_cast<std::uint8_t>(~BlackKingSide);
        }
    }
}

UndoState Board::make_move(const Move& move) {
    const Piece moved = piece_at(move.from);
    const Color moving_color = color_of(moved);
    const Square en_passant_capture_square = moving_color == Color::White ? move.to - 8 : move.to + 8;
    const Piece captured = (move.flags & EnPassant) != 0
        ? piece_at(en_passant_capture_square)
        : piece_at(move.to);

    UndoState undo{
        move,
        captured,
        castling_rights_,
        en_passant_square_,
        halfmove_clock_,
        fullmove_number_,
        side_to_move_,
    };

    en_passant_square_ = kNoSquare;
    ++halfmove_clock_;
    if (type_of(moved) == PieceType::Pawn || captured != Piece::None) {
        halfmove_clock_ = 0;
    }

    update_castling_rights_for_move(move.from, move.to, moved, captured);

    remove_piece(move.from);
    if ((move.flags & EnPassant) != 0) {
        remove_piece(en_passant_capture_square);
    } else if (captured != Piece::None) {
        remove_piece(move.to);
    }

    if ((move.flags & Promotion) != 0) {
        set_piece(move.to, make_piece(moving_color, move.promotion));
    } else {
        set_piece(move.to, moved);
    }

    if ((move.flags & KingCastle) != 0) {
        const int rank = moving_color == Color::White ? 0 : 7;
        move_piece(make_square(7, rank), make_square(5, rank));
    } else if ((move.flags & QueenCastle) != 0) {
        const int rank = moving_color == Color::White ? 0 : 7;
        move_piece(make_square(0, rank), make_square(3, rank));
    }

    if ((move.flags & DoublePawnPush) != 0) {
        en_passant_square_ = moving_color == Color::White ? move.from + 8 : move.from - 8;
    }

    if (moving_color == Color::Black) {
        ++fullmove_number_;
    }
    side_to_move_ = opposite(side_to_move_);
    return undo;
}

void Board::unmake_move(const UndoState& undo) {
    const Move& move = undo.move;
    side_to_move_ = undo.side_to_move;
    castling_rights_ = undo.castling_rights;
    en_passant_square_ = undo.en_passant_square;
    halfmove_clock_ = undo.halfmove_clock;
    fullmove_number_ = undo.fullmove_number;

    const Color moving_color = side_to_move_;
    const Piece moved = make_piece(moving_color, PieceType::Pawn);

    if ((move.flags & KingCastle) != 0) {
        const int rank = moving_color == Color::White ? 0 : 7;
        move_piece(make_square(5, rank), make_square(7, rank));
    } else if ((move.flags & QueenCastle) != 0) {
        const int rank = moving_color == Color::White ? 0 : 7;
        move_piece(make_square(3, rank), make_square(0, rank));
    }

    const Piece piece_on_to = piece_at(move.to);
    remove_piece(move.to);
    if ((move.flags & Promotion) != 0) {
        set_piece(move.from, moved);
    } else {
        set_piece(move.from, piece_on_to);
    }

    if ((move.flags & EnPassant) != 0) {
        const Square captured_square = moving_color == Color::White ? move.to - 8 : move.to + 8;
        set_piece(captured_square, undo.captured);
    } else if (undo.captured != Piece::None) {
        set_piece(move.to, undo.captured);
    }
}

}  // namespace chess

