#include "chess/core/attacks.h"

#include <cassert>

#include "chess/core/board.h"

namespace chess::attacks {

Bitboard attackers_to(const Board& board, Square square, Color by_color) {
    assert(is_valid_square(square));
    const Bitboard by_pawns = board.pieces(by_color, PieceType::Pawn);
    const Bitboard by_knights = board.pieces(by_color, PieceType::Knight);
    const Bitboard by_bishops = board.pieces(by_color, PieceType::Bishop);
    const Bitboard by_rooks = board.pieces(by_color, PieceType::Rook);
    const Bitboard by_queens = board.pieces(by_color, PieceType::Queen);
    const Bitboard by_king = board.pieces(by_color, PieceType::King);
    const Bitboard occupancy = board.occupancy();

    return (pawn_attacks(opposite(by_color), square) & by_pawns)
        | (knight_attacks(square) & by_knights)
        | (king_attacks(square) & by_king)
        | (bishop_attacks(square, occupancy) & (by_bishops | by_queens))
        | (rook_attacks(square, occupancy) & (by_rooks | by_queens));
}

bool is_square_attacked(const Board& board, Square square, Color by_color) {
    return attackers_to(board, square, by_color) != 0;
}

}  // namespace chess::attacks
