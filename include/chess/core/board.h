#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "chess/core/move.h"
#include "chess/core/types.h"

namespace chess {

struct UndoState {
    Move move;
    Piece captured = Piece::None;
    std::uint8_t castling_rights = 0;
    Square en_passant_square = kNoSquare;
    int halfmove_clock = 0;
    int fullmove_number = 1;
    Color side_to_move = Color::White;
};

class Board {
public:
    Board();

    static Board start_position();

    [[nodiscard]] Piece piece_at(Square square) const;
    [[nodiscard]] Color side_to_move() const;
    [[nodiscard]] std::uint8_t castling_rights() const;
    [[nodiscard]] Square en_passant_square() const;
    [[nodiscard]] int halfmove_clock() const;
    [[nodiscard]] int fullmove_number() const;
    [[nodiscard]] Bitboard occupancy(Color color) const;
    [[nodiscard]] Bitboard occupancy() const;
    [[nodiscard]] Bitboard pieces(Color color, PieceType type) const;
    [[nodiscard]] Square king_square(Color color) const;
    [[nodiscard]] bool is_square_attacked(Square square, Color by_color) const;
    [[nodiscard]] bool in_check(Color color) const;
    [[nodiscard]] std::string to_fen() const;

    void clear();
    void set_piece(Square square, Piece piece);
    void remove_piece(Square square);
    void set_side_to_move(Color color);
    void set_castling_rights(std::uint8_t rights);
    void set_en_passant_square(Square square);
    void set_halfmove_clock(int value);
    void set_fullmove_number(int value);

    UndoState make_move(const Move& move);
    void unmake_move(const UndoState& undo);

private:
    std::array<Piece, kBoardSquareCount> squares_{};
    std::array<std::array<Bitboard, 6>, 2> pieces_{};
    std::array<Bitboard, 2> occupancy_{};
    Bitboard all_occupancy_ = 0;
    Color side_to_move_ = Color::White;
    std::uint8_t castling_rights_ = 0;
    Square en_passant_square_ = kNoSquare;
    int halfmove_clock_ = 0;
    int fullmove_number_ = 1;

    void move_piece(Square from, Square to);
    void update_castling_rights_for_move(Square from, Square to, Piece moved, Piece captured);
};

}  // namespace chess

