#pragma once

#include "chess/core/board.h"

namespace chess::engine {

struct EvalTrace {
    int material = 0;
    int piece_square = 0;
    int mobility = 0;
    int safe_mobility = 0;
    int king_safety = 0;
    int threats = 0;
    int pawn_structure = 0;
    int outposts = 0;
    int rook_files = 0;
    int space = 0;
    int center_control = 0;
    int bishop_quality = 0;
    int pawn_dynamics = 0;
    int development = 0;
    int trade_context = 0;
    int total = 0;
};

int material_value(PieceType type);
EvalTrace evaluate_trace_white_perspective(const Board& board);
int evaluate_white_perspective(const Board& board);
int evaluate(const Board& board);

}  // namespace chess::engine
