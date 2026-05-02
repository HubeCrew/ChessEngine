#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "chess/core/types.h"

namespace chess {

enum MoveFlag : std::uint8_t {
    Quiet = 0,
    Capture = 1 << 0,
    DoublePawnPush = 1 << 1,
    KingCastle = 1 << 2,
    QueenCastle = 1 << 3,
    EnPassant = 1 << 4,
    Promotion = 1 << 5,
};

struct Move {
    Square from = kNoSquare;
    Square to = kNoSquare;
    PieceType promotion = PieceType::None;
    std::uint8_t flags = Quiet;

    [[nodiscard]] bool is_capture() const {
        return (flags & Capture) != 0;
    }

    [[nodiscard]] bool is_promotion() const {
        return (flags & Promotion) != 0;
    }

    [[nodiscard]] bool is_castling() const {
        return (flags & (KingCastle | QueenCastle)) != 0;
    }

    friend bool operator==(const Move&, const Move&) = default;
};

using MoveList = std::vector<Move>;

std::string move_to_uci(const Move& move);
PieceType promotion_from_uci(char value);
char promotion_to_uci(PieceType type);

}  // namespace chess

