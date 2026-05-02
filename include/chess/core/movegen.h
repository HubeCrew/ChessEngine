#pragma once

#include "chess/core/board.h"

namespace chess {

MoveList generate_pseudo_legal_moves(const Board& board);
MoveList generate_legal_moves(Board& board);
bool is_legal_move(Board& board, const Move& move);
Move parse_uci_move(Board& board, std::string_view text);

}  // namespace chess

