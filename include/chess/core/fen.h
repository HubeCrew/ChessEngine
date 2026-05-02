#pragma once

#include <string>

#include "chess/core/board.h"

namespace chess {

constexpr std::string_view kStartFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

Board board_from_fen(std::string_view fen);

}  // namespace chess

