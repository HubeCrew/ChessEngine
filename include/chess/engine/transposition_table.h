#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "chess/core/move.h"

namespace chess::engine {

enum class Bound : std::uint8_t {
    Exact,
    Lower,
    Upper,
};

struct TranspositionEntry {
    std::uint64_t key = 0;
    Move best_move;
    int depth = -1;
    int score = 0;
    Bound bound = Bound::Exact;
    std::uint8_t generation = 0;
    bool occupied = false;
};

class TranspositionTable {
public:
    explicit TranspositionTable(std::size_t megabytes = 64);

    void resize_mb(std::size_t megabytes);
    void clear();
    void new_search();

    [[nodiscard]] const TranspositionEntry* probe(std::uint64_t key) const;
    void store(std::uint64_t key, int depth, int score, Bound bound, Move best_move);

    [[nodiscard]] std::size_t size_mb() const;
    [[nodiscard]] std::size_t entry_count() const;

private:
    std::vector<TranspositionEntry> entries_;
    std::size_t size_mb_ = 0;
    std::uint8_t generation_ = 0;

    [[nodiscard]] std::size_t index_for(std::uint64_t key) const;
};

}  // namespace chess::engine

