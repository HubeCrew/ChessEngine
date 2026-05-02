#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "chess/core/board.h"
#include "chess/engine/transposition_table.h"

namespace chess::engine {

struct SearchLimits {
    int depth = 4;
    std::chrono::milliseconds move_time{0};
};

struct SearchResult {
    Move best_move;
    int score_centipawns = 0;
    int depth = 0;
    std::uint64_t nodes = 0;
    std::uint64_t tt_hits = 0;
    std::chrono::milliseconds elapsed{0};
    std::uint64_t nps = 0;
    std::vector<Move> principal_variation;
};

class Searcher {
public:
    Searcher();

    SearchResult search(Board& board, const SearchLimits& limits);
    void set_hash_size_mb(std::size_t megabytes);
    [[nodiscard]] std::size_t hash_size_mb() const;
    void clear();

private:
    static constexpr int kMaxPly = 128;

    std::uint64_t nodes_ = 0;
    std::uint64_t tt_hits_ = 0;
    std::chrono::steady_clock::time_point deadline_{};
    std::chrono::steady_clock::time_point start_time_{};
    bool use_deadline_ = false;
    TranspositionTable tt_;
    std::array<std::array<Move, 2>, kMaxPly> killer_moves_{};
    std::array<std::array<std::array<int, 64>, 64>, 2> history_{};
    std::vector<Move> previous_iteration_pv_;

    int negamax(Board& board, int depth, int ply, int alpha, int beta);
    int quiescence(Board& board, int ply, int alpha, int beta);
    void order_moves(Board& board, MoveList& moves, const Move& tt_move, int ply) const;
    void age_history();
    void record_cutoff(
        const Move& move,
        int depth,
        int ply,
        Color side_to_move,
        const Move* quiets_tried_before_cutoff,
        std::size_t quiet_count
    );
    std::vector<Move> extract_principal_variation(Board& board, int depth) const;
    bool should_stop() const;
};

}  // namespace chess::engine
