#include "chess/core/board.h"

#include <cassert>

#include "chess/core/attacks.h"
#include "chess/core/fen.h"
#include "chess/core/zobrist.h"

namespace chess {

namespace {

constexpr int color_index(Color color) {
    return static_cast<int>(color);
}

constexpr int type_index(PieceType type) {
    return static_cast<int>(type);
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
    return attacks::is_square_attacked(*this, square, by_color);
}

bool Board::in_check(Color color) const {
    const Square king = king_square(color);
    return is_valid_square(king) && is_square_attacked(king, opposite(color));
}

std::uint64_t Board::hash_key() const {
    return hash_key_;
}

std::uint64_t Board::recompute_hash() const {
    std::uint64_t key = 0;
    for (Square square = 0; square < kBoardSquareCount; ++square) {
        key ^= zobrist::piece_square(piece_at(square), square);
    }
    if (side_to_move_ == Color::Black) {
        key ^= zobrist::side_to_move();
    }
    key ^= zobrist::castling(castling_rights_);
    if (en_passant_square_ != kNoSquare) {
        key ^= zobrist::en_passant_file(file_of(en_passant_square_));
    }
    return key;
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
    hash_key_ = zobrist::castling(castling_rights_);
}

void Board::set_piece(Square square, Piece piece) {
    assert(is_valid_square(square));
    if (piece == Piece::None) {
        remove_piece(square);
        return;
    }

    take_piece(square);
    put_piece(square, piece);
}

void Board::remove_piece(Square square) {
    assert(is_valid_square(square));
    take_piece(square);
}

void Board::set_side_to_move(Color color) {
    if (side_to_move_ != color) {
        hash_key_ ^= zobrist::side_to_move();
        side_to_move_ = color;
    }
}

void Board::set_castling_rights(std::uint8_t rights) {
    const std::uint8_t normalized = rights & 0x0fU;
    if (castling_rights_ == normalized) {
        return;
    }

    hash_key_ ^= zobrist::castling(castling_rights_);
    castling_rights_ = normalized;
    hash_key_ ^= zobrist::castling(castling_rights_);
}

void Board::set_en_passant_square(Square square) {
    if (en_passant_square_ == square) {
        return;
    }

    if (en_passant_square_ != kNoSquare) {
        hash_key_ ^= zobrist::en_passant_file(file_of(en_passant_square_));
    }
    en_passant_square_ = square;
    if (en_passant_square_ != kNoSquare) {
        hash_key_ ^= zobrist::en_passant_file(file_of(en_passant_square_));
    }
}

void Board::set_halfmove_clock(int value) {
    halfmove_clock_ = value;
}

void Board::set_fullmove_number(int value) {
    fullmove_number_ = value;
}

void Board::put_piece(Square square, Piece piece) {
    assert(is_valid_square(square));
    assert(piece != Piece::None);
    assert(squares_[square] == Piece::None);

    const auto color = color_of(piece);
    const auto type = type_of(piece);
    const Bitboard bit = square_bb(square);

    squares_[square] = piece;
    pieces_[color_index(color)][type_index(type)] |= bit;
    occupancy_[color_index(color)] |= bit;
    all_occupancy_ |= bit;
    hash_key_ ^= zobrist::piece_square(piece, square);
}

Piece Board::take_piece(Square square) {
    assert(is_valid_square(square));
    const Piece piece = squares_[square];
    if (piece == Piece::None) {
        return Piece::None;
    }

    const auto color = color_of(piece);
    const auto type = type_of(piece);
    const Bitboard bit = square_bb(square);
    pieces_[color_index(color)][type_index(type)] &= ~bit;
    occupancy_[color_index(color)] &= ~bit;
    all_occupancy_ &= ~bit;
    squares_[square] = Piece::None;
    hash_key_ ^= zobrist::piece_square(piece, square);
    return piece;
}

void Board::move_piece_unchecked(Square from, Square to) {
    const Piece piece = take_piece(from);
    assert(piece != Piece::None);
    put_piece(to, piece);
}

void Board::refresh_hash() {
    hash_key_ = recompute_hash();
}

void Board::update_castling_rights_for_move(Square from, Square to, Piece moved, Piece captured) {
    std::uint8_t rights = castling_rights_;

    if (type_of(moved) == PieceType::King) {
        if (color_of(moved) == Color::White) {
            rights &= static_cast<std::uint8_t>(~(WhiteKingSide | WhiteQueenSide));
        } else {
            rights &= static_cast<std::uint8_t>(~(BlackKingSide | BlackQueenSide));
        }
    }

    if (type_of(moved) == PieceType::Rook) {
        if (from == make_square(0, 0)) {
            rights &= static_cast<std::uint8_t>(~WhiteQueenSide);
        } else if (from == make_square(7, 0)) {
            rights &= static_cast<std::uint8_t>(~WhiteKingSide);
        } else if (from == make_square(0, 7)) {
            rights &= static_cast<std::uint8_t>(~BlackQueenSide);
        } else if (from == make_square(7, 7)) {
            rights &= static_cast<std::uint8_t>(~BlackKingSide);
        }
    }

    if (type_of(captured) == PieceType::Rook) {
        if (to == make_square(0, 0)) {
            rights &= static_cast<std::uint8_t>(~WhiteQueenSide);
        } else if (to == make_square(7, 0)) {
            rights &= static_cast<std::uint8_t>(~WhiteKingSide);
        } else if (to == make_square(0, 7)) {
            rights &= static_cast<std::uint8_t>(~BlackQueenSide);
        } else if (to == make_square(7, 7)) {
            rights &= static_cast<std::uint8_t>(~BlackKingSide);
        }
    }

    set_castling_rights(rights);
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
        hash_key_,
        castling_rights_,
        en_passant_square_,
        halfmove_clock_,
        fullmove_number_,
        side_to_move_,
    };

    set_en_passant_square(kNoSquare);
    ++halfmove_clock_;
    if (type_of(moved) == PieceType::Pawn || captured != Piece::None) {
        halfmove_clock_ = 0;
    }

    update_castling_rights_for_move(move.from, move.to, moved, captured);

    take_piece(move.from);
    if ((move.flags & EnPassant) != 0) {
        take_piece(en_passant_capture_square);
    } else if (captured != Piece::None) {
        take_piece(move.to);
    }

    if ((move.flags & Promotion) != 0) {
        put_piece(move.to, make_piece(moving_color, move.promotion));
    } else {
        put_piece(move.to, moved);
    }

    if ((move.flags & KingCastle) != 0) {
        const int rank = moving_color == Color::White ? 0 : 7;
        move_piece_unchecked(make_square(7, rank), make_square(5, rank));
    } else if ((move.flags & QueenCastle) != 0) {
        const int rank = moving_color == Color::White ? 0 : 7;
        move_piece_unchecked(make_square(0, rank), make_square(3, rank));
    }

    if ((move.flags & DoublePawnPush) != 0) {
        set_en_passant_square(moving_color == Color::White ? move.from + 8 : move.from - 8);
    }

    if (moving_color == Color::Black) {
        ++fullmove_number_;
    }
    set_side_to_move(opposite(side_to_move_));
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
        move_piece_unchecked(make_square(5, rank), make_square(7, rank));
    } else if ((move.flags & QueenCastle) != 0) {
        const int rank = moving_color == Color::White ? 0 : 7;
        move_piece_unchecked(make_square(3, rank), make_square(0, rank));
    }

    const Piece piece_on_to = piece_at(move.to);
    take_piece(move.to);
    if ((move.flags & Promotion) != 0) {
        put_piece(move.from, moved);
    } else {
        put_piece(move.from, piece_on_to);
    }

    if ((move.flags & EnPassant) != 0) {
        const Square captured_square = moving_color == Color::White ? move.to - 8 : move.to + 8;
        put_piece(captured_square, undo.captured);
    } else if (undo.captured != Piece::None) {
        put_piece(move.to, undo.captured);
    }

    hash_key_ = undo.hash_key;
    assert(hash_key_ == recompute_hash());
}

UndoState Board::make_null_move() {
    assert(!in_check(side_to_move_));

    UndoState undo{
        Move{},
        Piece::None,
        hash_key_,
        castling_rights_,
        en_passant_square_,
        halfmove_clock_,
        fullmove_number_,
        side_to_move_,
    };

    set_en_passant_square(kNoSquare);
    ++halfmove_clock_;
    if (side_to_move_ == Color::Black) {
        ++fullmove_number_;
    }
    set_side_to_move(opposite(side_to_move_));
    return undo;
}

void Board::unmake_null_move(const UndoState& undo) {
    side_to_move_ = undo.side_to_move;
    castling_rights_ = undo.castling_rights;
    en_passant_square_ = undo.en_passant_square;
    halfmove_clock_ = undo.halfmove_clock;
    fullmove_number_ = undo.fullmove_number;
    hash_key_ = undo.hash_key;
    assert(hash_key_ == recompute_hash());
}

}  // namespace chess
