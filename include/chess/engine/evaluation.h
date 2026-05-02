#pragma once

#include "chess/core/board.h"

namespace chess::engine {

int material_value(PieceType type);
int evaluate_white_perspective(const Board& board);
int evaluate(const Board& board);

}  // namespace chess::engine

