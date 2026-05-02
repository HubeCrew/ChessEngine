#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "chess/core/board.h"

namespace chess {

enum class GameOutcome : std::uint8_t {
    Ongoing,
    WhiteWin,
    BlackWin,
    Draw,
};

enum class GameReason : std::uint8_t {
    None,
    Checkmate,
    Stalemate,
    FiftyMoveRule,
    ThreefoldRepetition,
    InsufficientMaterial,
};

struct GameStatus {
    GameOutcome outcome = GameOutcome::Ongoing;
    GameReason reason = GameReason::None;
    int legal_moves = 0;
};

[[nodiscard]] GameStatus adjudicate(Board& board, const std::vector<std::uint64_t>& repetition_history);
[[nodiscard]] bool has_insufficient_material(const Board& board);
[[nodiscard]] std::string_view to_string(GameOutcome outcome);
[[nodiscard]] std::string_view to_string(GameReason reason);

}  // namespace chess
