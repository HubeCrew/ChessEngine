#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "chess/core/move.h"

namespace chess::engine {

constexpr int kNoTranspositionStaticEval = 2'000'000;

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
    int static_eval = kNoTranspositionStaticEval;
    Bound bound = Bound::Exact;
    std::uint8_t generation = 0;
    bool occupied = false;
};

class TranspositionTable {
public:
    static constexpr std::size_t kClusterSize = 4;

    explicit TranspositionTable(std::size_t megabytes = 64);

    void resize_mb(std::size_t megabytes);
    void clear();
    void new_search();

    [[nodiscard]] const TranspositionEntry* probe(std::uint64_t key) const;
    void store(
        std::uint64_t key,
        int depth,
        int score,
        Bound bound,
        Move best_move,
        int static_eval = kNoTranspositionStaticEval
    );

    [[nodiscard]] std::size_t size_mb() const;
    [[nodiscard]] std::size_t entry_count() const;

private:
    struct Cluster {
        std::array<TranspositionEntry, kClusterSize> entries{};
    };

    std::vector<Cluster> clusters_;
    std::size_t size_mb_ = 0;
    std::uint8_t generation_ = 0;

    [[nodiscard]] std::size_t cluster_index_for(std::uint64_t key) const;
    [[nodiscard]] int generation_distance(std::uint8_t generation) const;
};

}  // namespace chess::engine
