#include "chess/engine/transposition_table.h"

#include <algorithm>

namespace chess::engine {

namespace {

constexpr std::size_t kBytesPerMegabyte = 1024 * 1024;

}  // namespace

TranspositionTable::TranspositionTable(std::size_t megabytes) {
    resize_mb(megabytes);
}

void TranspositionTable::resize_mb(std::size_t megabytes) {
    size_mb_ = std::max<std::size_t>(1, megabytes);
    const std::size_t bytes = size_mb_ * kBytesPerMegabyte;
    const std::size_t entries = std::max<std::size_t>(1, bytes / sizeof(TranspositionEntry));
    entries_.assign(entries, TranspositionEntry{});
    generation_ = 0;
}

void TranspositionTable::clear() {
    std::fill(entries_.begin(), entries_.end(), TranspositionEntry{});
    generation_ = 0;
}

void TranspositionTable::new_search() {
    ++generation_;
    if (generation_ == 0) {
        generation_ = 1;
    }
}

const TranspositionEntry* TranspositionTable::probe(std::uint64_t key) const {
    if (entries_.empty()) {
        return nullptr;
    }

    const TranspositionEntry& entry = entries_[index_for(key)];
    if (!entry.occupied || entry.key != key) {
        return nullptr;
    }
    return &entry;
}

void TranspositionTable::store(std::uint64_t key, int depth, int score, Bound bound, Move best_move) {
    if (entries_.empty()) {
        return;
    }

    TranspositionEntry& entry = entries_[index_for(key)];
    const bool replace =
        !entry.occupied
        || entry.generation != generation_
        || depth >= entry.depth;

    if (!replace) {
        return;
    }

    entry = TranspositionEntry{
        key,
        best_move,
        depth,
        score,
        bound,
        generation_,
        true,
    };
}

std::size_t TranspositionTable::size_mb() const {
    return size_mb_;
}

std::size_t TranspositionTable::entry_count() const {
    return entries_.size();
}

std::size_t TranspositionTable::index_for(std::uint64_t key) const {
    return static_cast<std::size_t>(key % entries_.size());
}

}  // namespace chess::engine
