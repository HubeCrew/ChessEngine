#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "chess/core/board.h"
#include "chess/engine/evaluation.h"
#include "chess/engine/transposition_table.h"

namespace chess::engine {

struct SearchLimits {
    int depth = 4;
    std::chrono::milliseconds move_time{0};
    std::chrono::milliseconds white_time{0};
    std::chrono::milliseconds black_time{0};
    std::chrono::milliseconds white_increment{0};
    std::chrono::milliseconds black_increment{0};
    int moves_to_go = 0;
    std::vector<Move> search_moves;
};

struct SearchDiagnostics {
    std::uint64_t evaluations = 0;
    std::uint64_t evaluation_ns = 0;
    std::uint64_t nnue_stack_evaluations = 0;
    std::uint64_t nnue_stack_evaluation_ns = 0;
    std::uint64_t classical_evaluations = 0;
    std::uint64_t classical_evaluation_ns = 0;
    std::uint64_t nnue_accumulator_refreshes = 0;
    std::uint64_t nnue_accumulator_refresh_ns = 0;
    std::uint64_t nnue_accumulator_update_attempts = 0;
    std::uint64_t nnue_accumulator_update_successes = 0;
    std::uint64_t nnue_accumulator_update_fallbacks = 0;
    std::uint64_t nnue_accumulator_update_ns = 0;
    std::uint64_t nnue_accumulator_null_copies = 0;
    std::uint64_t move_picker_pv_picks = 0;
    std::uint64_t move_picker_tt_picks = 0;
    std::uint64_t move_picker_scored_moves = 0;
    std::uint64_t move_picker_searched_moves = 0;
    std::uint64_t move_picker_tactical_picks = 0;
    std::uint64_t move_picker_killer_picks = 0;
    std::uint64_t move_picker_quiet_picks = 0;
    std::uint64_t beta_cutoffs = 0;
    std::uint64_t beta_cutoff_move_index_sum = 0;
    std::uint64_t illegal_pseudo_moves = 0;
    std::uint64_t null_move_attempts = 0;
    std::uint64_t null_move_cutoffs = 0;
    std::uint64_t lmr_reductions = 0;
    std::uint64_t lmr_researches = 0;
    std::uint64_t qsearch_in_check_nodes = 0;
    std::uint64_t qsearch_stand_pat_nodes = 0;
    std::uint64_t see_calls = 0;
    std::uint64_t move_gives_check_calls = 0;
};

struct SearchResult {
    Move best_move;
    int score_centipawns = 0;
    int depth = 0;
    std::uint64_t nodes = 0;
    std::uint64_t qnodes = 0;
    std::uint64_t tt_hits = 0;
    std::chrono::milliseconds elapsed{0};
    std::uint64_t nps = 0;
    std::vector<Move> principal_variation;
    SearchDiagnostics diagnostics;
};

[[nodiscard]] bool move_gives_check(const Board& board, const Move& move);

class Searcher {
public:
    Searcher();

    SearchResult search(Board& board, const SearchLimits& limits);
    void set_hash_size_mb(std::size_t megabytes);
    [[nodiscard]] std::size_t hash_size_mb() const;
    void set_null_move_pruning(bool enabled);
    [[nodiscard]] bool null_move_pruning() const;
    void set_search_extensions(bool enabled);
    [[nodiscard]] bool search_extensions() const;
    void set_profiling(bool enabled);
    [[nodiscard]] bool profiling() const;
    void set_eval_type(EvalType type);
    [[nodiscard]] EvalType eval_type() const;
    [[nodiscard]] bool load_nnue(const std::filesystem::path& path, std::string* error = nullptr);
    void clear_nnue();
    [[nodiscard]] bool nnue_loaded() const;
    [[nodiscard]] const nnue::ModelInfo& nnue_info() const;
    [[nodiscard]] int evaluate_nnue_white_perspective(const Board& board) const;
    [[nodiscard]] int evaluate_white_perspective(const Board& board) const;
    [[nodiscard]] int evaluate_side_to_move(const Board& board) const;
    void clear();

private:
    static constexpr int kMaxPly = 128;

    std::uint64_t nodes_ = 0;
    std::uint64_t qnodes_ = 0;
    std::uint64_t tt_hits_ = 0;
    std::chrono::steady_clock::time_point deadline_{};
    std::chrono::steady_clock::time_point start_time_{};
    bool use_deadline_ = false;
    bool null_move_pruning_ = true;
    bool search_extensions_ = true;
    bool profiling_ = false;
    EvalType eval_type_ = EvalType::Classical;
    nnue::Network nnue_;
    SearchDiagnostics diagnostics_{};
    TranspositionTable tt_;
    std::array<std::array<Move, 2>, kMaxPly> killer_moves_{};
    std::array<std::array<std::array<int, 64>, 64>, 2> history_{};
    std::array<nnue::QuantizedAccumulatorPair, kMaxPly> nnue_accumulator_stack_{};
    std::vector<Move> previous_iteration_pv_;
    MoveList root_search_moves_;
    bool root_search_moves_constrained_ = false;

    int negamax(Board& board, int depth, int ply, int alpha, int beta, bool allow_null_move, int extensions_used);
    int quiescence(Board& board, int ply, int alpha, int beta, int qply);
    int evaluate_with_diagnostics(const Board& board, int ply);
    int static_exchange_with_diagnostics(const Board& board, const Move& move);
    void refresh_nnue_accumulator(Board& board, int ply);
    void update_nnue_accumulator_after_move(const Board& board, const UndoState& undo, int parent_ply, int child_ply);
    void update_nnue_accumulator_after_null_move(int parent_ply, int child_ply);
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
