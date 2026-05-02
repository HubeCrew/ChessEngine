#pragma once

#include <chrono>
#include <optional>

#include "chess/core/board.h"

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
};

class Searcher {
public:
    SearchResult search(Board& board, const SearchLimits& limits);

private:
    std::uint64_t nodes_ = 0;
    std::chrono::steady_clock::time_point deadline_{};
    bool use_deadline_ = false;

    int negamax(Board& board, int depth, int alpha, int beta);
    int quiescence(Board& board, int alpha, int beta);
    int evaluate(const Board& board) const;
    bool should_stop() const;
};

}  // namespace chess::engine

