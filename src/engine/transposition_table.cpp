#include "chess/engine/transposition_table.h"

#include <algorithm>
#include <limits>

namespace chess::engine {

namespace {

constexpr std::size_t kBytesPerMegabyte = 1024 * 1024;
constexpr std::uint64_t kHashMultiplier = 11400714819323198485ULL;

std::size_t floor_power_of_two(std::size_t value) {
    std::size_t result = 1;
    while (result <= value / 2) {
        result *= 2;
    }
    return result;
}

int replacement_score(const TranspositionEntry& entry, int age) {
    const int bound_bonus = entry.bound == Bound::Exact ? 4 : 0;
    return entry.depth + bound_bonus - age * 8;
}

}  // namespace

TranspositionTable::TranspositionTable(std::size_t megabytes) {
    resize_mb(megabytes);
}

void TranspositionTable::resize_mb(std::size_t megabytes) {
    size_mb_ = std::max<std::size_t>(1, megabytes);
    const std::size_t bytes = size_mb_ * kBytesPerMegabyte;
    const std::size_t requested_clusters = std::max<std::size_t>(1, bytes / sizeof(Cluster));
    const std::size_t cluster_count = floor_power_of_two(requested_clusters);
    clusters_.assign(cluster_count, Cluster{});
    generation_ = 0;
}

void TranspositionTable::clear() {
    std::fill(clusters_.begin(), clusters_.end(), Cluster{});
    generation_ = 0;
}

void TranspositionTable::new_search() {
    ++generation_;
    if (generation_ == 0) {
        generation_ = 1;
    }
}

const TranspositionEntry* TranspositionTable::probe(std::uint64_t key) const {
    if (clusters_.empty()) {
        return nullptr;
    }

    const Cluster& cluster = clusters_[cluster_index_for(key)];
    for (const TranspositionEntry& entry : cluster.entries) {
        if (entry.occupied && entry.key == key) {
            return &entry;
        }
    }
    return nullptr;
}

void TranspositionTable::store(
    std::uint64_t key,
    int depth,
    int score,
    Bound bound,
    Move best_move,
    int static_eval
) {
    if (clusters_.empty()) {
        return;
    }

    Cluster& cluster = clusters_[cluster_index_for(key)];
    TranspositionEntry* victim = &cluster.entries.front();
    int victim_score = replacement_score(*victim, victim->occupied ? generation_distance(victim->generation) : 255);

    for (TranspositionEntry& entry : cluster.entries) {
        if (entry.occupied && entry.key == key) {
            const bool replace_existing = bound == Bound::Exact
                || depth >= entry.depth
                || entry.generation != generation_;
            if (!replace_existing) {
                entry.generation = generation_;
                if (is_valid_square(best_move.from) && is_valid_square(best_move.to)) {
                    entry.best_move = best_move;
                }
                if (static_eval != kNoTranspositionStaticEval) {
                    entry.static_eval = static_eval;
                }
                return;
            }

            entry = TranspositionEntry{
                key,
                best_move,
                depth,
                score,
                static_eval,
                bound,
                generation_,
                true,
            };
            return;
        }

        if (!entry.occupied) {
            victim = &entry;
            victim_score = std::numeric_limits<int>::min();
            break;
        }

        const int score_for_entry = replacement_score(entry, generation_distance(entry.generation));
        if (score_for_entry < victim_score) {
            victim = &entry;
            victim_score = score_for_entry;
        }
    }

    *victim = TranspositionEntry{
        key,
        best_move,
        depth,
        score,
        static_eval,
        bound,
        generation_,
        true,
    };
}

std::size_t TranspositionTable::size_mb() const {
    return size_mb_;
}

std::size_t TranspositionTable::entry_count() const {
    return clusters_.size() * kClusterSize;
}

std::size_t TranspositionTable::cluster_index_for(std::uint64_t key) const {
    return static_cast<std::size_t>((key * kHashMultiplier) & (clusters_.size() - 1));
}

int TranspositionTable::generation_distance(std::uint8_t generation) const {
    return static_cast<int>(static_cast<std::uint8_t>(generation_ - generation));
}

}  // namespace chess::engine
