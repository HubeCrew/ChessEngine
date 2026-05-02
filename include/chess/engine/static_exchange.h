#pragma once

#include "chess/core/board.h"

namespace chess::engine {

[[nodiscard]] int static_exchange_eval(const Board& board, const Move& move);

}  // namespace chess::engine
