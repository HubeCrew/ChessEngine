#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "chess/core/board.h"
#include "chess/core/move.h"

namespace chess::engine::book {

struct Entry {
    std::uint64_t key = 0;
    std::uint16_t move = 0;
    std::uint16_t weight = 0;
    std::uint32_t learn = 0;
};

struct ProbeMove {
    Move move;
    std::uint16_t weight = 0;
    std::uint32_t learn = 0;
    std::uint16_t raw_move = 0;
};

class OpeningBook {
public:
    [[nodiscard]] bool load(const std::filesystem::path& path, std::string* error = nullptr);
    void clear();

    [[nodiscard]] bool loaded() const;
    [[nodiscard]] const std::filesystem::path& path() const;
    [[nodiscard]] std::size_t entry_count() const;

    [[nodiscard]] std::optional<ProbeMove> probe(
        Board& board,
        const std::vector<Move>& allowed_moves,
        bool best_book_move,
        std::uint16_t minimum_weight
    );

private:
    std::filesystem::path path_;
    std::vector<Entry> entries_;
    std::uint64_t rng_state_ = 0x9e3779b97f4a7c15ULL;
};

[[nodiscard]] std::uint64_t polyglot_hash(const Board& board);
[[nodiscard]] std::optional<Move> decode_polyglot_move(Board& board, std::uint16_t raw_move);

}  // namespace chess::engine::book
