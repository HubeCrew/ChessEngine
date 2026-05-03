#pragma once

#include "chess/core/types.h"

namespace chess {

class Board;

namespace attacks {

[[nodiscard]] Bitboard pawn_attacks(Color color, Square square);
[[nodiscard]] Bitboard knight_attacks(Square square);
[[nodiscard]] Bitboard king_attacks(Square square);
[[nodiscard]] Bitboard bishop_attacks(Square square, Bitboard occupancy);
[[nodiscard]] Bitboard rook_attacks(Square square, Bitboard occupancy);
[[nodiscard]] Bitboard queen_attacks(Square square, Bitboard occupancy);
[[nodiscard]] Bitboard between(Square from, Square to);
[[nodiscard]] Bitboard attackers_to(const Board& board, Square square, Color by_color);
[[nodiscard]] bool is_square_attacked(const Board& board, Square square, Color by_color);

}  // namespace attacks

}  // namespace chess
