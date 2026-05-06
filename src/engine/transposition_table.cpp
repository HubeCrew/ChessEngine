#include "chess/engine/transposition_table.h"

#include <algorithm>
#include <limits>
#include <mutex>

namespace chess::engine {

namespace {

constexpr std::size_t kBytesPerMegabyte = 1024 * 1024;

bool has_usable_move(const Move& move) {
    return is_valid_square(move.from) && is_valid_square(move.to);
}

int replacement_score(const TranspositionEntry& entry, int age) {
    const int bound_bonus = entry.bound == Bound::Exact ? 10 : (entry.bound == Bound::Lower ? 2 : 0);
    const int move_bonus = has_usable_move(entry.best_move) ? 8 : -12;
    return entry.depth * 2 + bound_bonus + move_bonus - age * 16;
}

}  // namespace

TranspositionTable::TranspositionTable(std::size_t megabytes) {
    resize_mb(megabytes);
}

void TranspositionTable::resize_mb(std::size_t megabytes) {
    size_mb_ = std::max<std::size_t>(1, megabytes);
    const std::size_t bytes = size_mb_ * kBytesPerMegabyte;
    const std::size_t clusters = std::max<std::size_t>(1, bytes / sizeof(Cluster));
    clusters_.assign(clusters, Cluster{});
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
    thread_local TranspositionEntry entry_copy;
    if (clusters_.empty()) {
        return nullptr;
    }

    const std::size_t cluster_index = cluster_index_for(key);
    std::shared_lock lock(lock_for_cluster(cluster_index));
    const Cluster& cluster = clusters_[cluster_index];
    for (const TranspositionEntry& entry : cluster.entries) {
        if (entry.occupied && entry.key == key) {
            entry_copy = entry;
            return &entry_copy;
        }
    }
    return nullptr;
}

std::optional<TranspositionEntry> TranspositionTable::probe_entry(std::uint64_t key) const {
    if (clusters_.empty()) {
        return std::nullopt;
    }

    const std::size_t cluster_index = cluster_index_for(key);
    std::shared_lock lock(lock_for_cluster(cluster_index));
    const Cluster& cluster = clusters_[cluster_index];
    for (const TranspositionEntry& entry : cluster.entries) {
        if (entry.occupied && entry.key == key) {
            return entry;
        }
    }
    return std::nullopt;
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

    const std::size_t cluster_index = cluster_index_for(key);
    std::unique_lock lock(lock_for_cluster(cluster_index));
    Cluster& cluster = clusters_[cluster_index];
    const bool storing_move = has_usable_move(best_move);
    TranspositionEntry* victim = &cluster.entries.front();
    int victim_score = replacement_score(*victim, victim->occupied ? generation_distance(victim->generation) : 255);

    for (TranspositionEntry& entry : cluster.entries) {
        if (entry.occupied && entry.key == key) {
            const bool replace_existing = bound == Bound::Exact
                || depth >= entry.depth
                || entry.generation != generation_;
            if (!replace_existing) {
                entry.generation = generation_;
                if (storing_move) {
                    entry.best_move = best_move;
                }
                if (static_eval != kNoTranspositionStaticEval) {
                    entry.static_eval = static_eval;
                }
                return;
            }

            const Move retained_move = storing_move ? best_move : entry.best_move;
            entry = TranspositionEntry{
                key,
                retained_move,
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

    if (!storing_move && victim->occupied && has_usable_move(victim->best_move) && victim->generation == generation_ && depth + 4 < victim->depth) {
        return;
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
    return static_cast<std::size_t>(key % clusters_.size());
}

std::shared_mutex& TranspositionTable::lock_for_cluster(std::size_t cluster_index) const {
    return locks_[cluster_index % locks_.size()];
}

int TranspositionTable::generation_distance(std::uint8_t generation) const {
    return static_cast<int>(static_cast<std::uint8_t>(generation_ - generation));
}

}  // namespace chess::engine
