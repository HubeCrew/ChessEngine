#pragma once

#include <cstdint>

#include "chess/core/types.h"

namespace chess::zobrist {

std::uint64_t piece_square(Piece piece, Square square);
std::uint64_t side_to_move();
std::uint64_t castling(std::uint8_t rights);
std::uint64_t en_passant_file(int file);

}  // namespace chess::zobrist

