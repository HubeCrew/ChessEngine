#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

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

class MoveList {
public:
    static constexpr std::size_t kCapacity = 256;

    using iterator = Move*;
    using const_iterator = const Move*;

    void reserve(std::size_t capacity) {
        assert(capacity <= kCapacity);
        (void)capacity;
    }

    void push_back(const Move& move) {
        assert(size_ < moves_.size());
        moves_[size_] = move;
        ++size_;
    }

    void clear() {
        size_ = 0;
    }

    [[nodiscard]] bool empty() const {
        return size_ == 0;
    }

    [[nodiscard]] std::size_t size() const {
        return size_;
    }

    [[nodiscard]] Move& operator[](std::size_t index) {
        assert(index < size_);
        return moves_[index];
    }

    [[nodiscard]] const Move& operator[](std::size_t index) const {
        assert(index < size_);
        return moves_[index];
    }

    [[nodiscard]] Move& front() {
        assert(size_ > 0);
        return moves_[0];
    }

    [[nodiscard]] const Move& front() const {
        assert(size_ > 0);
        return moves_[0];
    }

    [[nodiscard]] iterator begin() {
        return moves_.data();
    }

    [[nodiscard]] iterator end() {
        return moves_.data() + size_;
    }

    [[nodiscard]] const_iterator begin() const {
        return moves_.data();
    }

    [[nodiscard]] const_iterator end() const {
        return moves_.data() + size_;
    }

    [[nodiscard]] const_iterator cbegin() const {
        return begin();
    }

    [[nodiscard]] const_iterator cend() const {
        return end();
    }

private:
    std::array<Move, kCapacity> moves_{};
    std::size_t size_ = 0;
};

std::string move_to_uci(const Move& move);
PieceType promotion_from_uci(char value);
char promotion_to_uci(PieceType type);

}  // namespace chess
