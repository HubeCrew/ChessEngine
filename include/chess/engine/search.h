#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
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
    std::uint64_t nnue_refresh_placement_ns = 0;
    std::uint64_t nnue_refresh_threat_ns = 0;
    std::uint64_t nnue_update_placement_ns = 0;
    std::uint64_t nnue_update_threat_ns = 0;
    std::uint64_t nnue_dense_convert_ns = 0;
    std::uint64_t nnue_dense_forward_ns = 0;
    std::uint64_t nnue_dirty_squares = 0;
    std::uint64_t nnue_dirty_attackers_before = 0;
    std::uint64_t nnue_dirty_attackers_after = 0;
    std::uint64_t nnue_dirty_threat_removed = 0;
    std::uint64_t nnue_dirty_threat_added = 0;
    std::uint64_t nnue_dirty_threat_unchanged = 0;
    std::uint64_t nnue_fallback_parent_invalid = 0;
    std::uint64_t nnue_fallback_unsupported = 0;
    std::uint64_t nnue_fallback_invalid_move = 0;
    std::uint64_t nnue_fallback_missing_king = 0;
    std::uint64_t nnue_fallback_moving_king = 0;
    std::uint64_t nnue_fallback_placement_feature = 0;
    std::uint64_t nnue_fallback_dirty_overflow = 0;
    std::uint64_t nnue_partial_refreshes = 0;
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
    std::uint64_t reverse_futility_prunes = 0;
    std::uint64_t razoring_prunes = 0;
    std::uint64_t probcut_attempts = 0;
    std::uint64_t probcut_cutoffs = 0;
    std::uint64_t futility_prunes = 0;
    std::uint64_t late_move_prunes = 0;
    std::uint64_t lmr_reductions = 0;
    std::uint64_t lmr_researches = 0;
    std::uint64_t singular_extension_attempts = 0;
    std::uint64_t singular_extensions = 0;
    std::uint64_t correction_history_updates = 0;
    std::uint64_t qsearch_in_check_nodes = 0;
    std::uint64_t qsearch_stand_pat_nodes = 0;
    std::uint64_t see_calls = 0;
    std::uint64_t move_gives_check_calls = 0;
};

struct SearchResult {
    struct Line {
        int multipv = 1;
        Move best_move;
        int score_centipawns = 0;
        std::vector<Move> principal_variation;
    };

    Move best_move;
    int score_centipawns = 0;
    int depth = 0;
    std::uint64_t nodes = 0;
    std::uint64_t qnodes = 0;
    std::uint64_t tt_hits = 0;
    std::chrono::milliseconds elapsed{0};
    std::uint64_t nps = 0;
    std::vector<Move> principal_variation;
    std::vector<Line> lines;
    SearchDiagnostics diagnostics;
};

struct RootMoveInfo {
    Move move;
    int score = 0;
    int previous_score = 0;
    int average_score = 0;
    std::uint64_t nodes = 0;
    std::uint64_t effort = 0;
    bool searched = false;
    std::vector<Move> principal_variation;
};

struct TimeManagementConfig {
    std::chrono::milliseconds move_overhead{20};
    int slow_mover = 100;
};

[[nodiscard]] bool move_gives_check(const Board& board, const Move& move);

class TimeManager {
public:
    void clear();
    void set_config(TimeManagementConfig config);
    [[nodiscard]] TimeManagementConfig config() const;
    void init(const Board& board, const SearchLimits& limits);
    [[nodiscard]] bool active() const;
    [[nodiscard]] std::chrono::milliseconds optimum() const;
    [[nodiscard]] std::chrono::milliseconds maximum() const;
    [[nodiscard]] std::chrono::steady_clock::time_point deadline(std::chrono::steady_clock::time_point start) const;
    [[nodiscard]] bool should_stop_after_iteration(
        std::chrono::milliseconds elapsed,
        int completed_depth,
        int best_move_stability,
        int best_move_changes,
        int previous_score,
        int best_score,
        int previous_average_score,
        std::uint64_t best_effort,
        std::uint64_t total_nodes
    );

private:
    TimeManagementConfig config_{};
    std::chrono::milliseconds optimum_{0};
    std::chrono::milliseconds maximum_{0};
    double previous_time_reduction_ = 1.0;
};

using HistoryTable = std::array<std::array<std::array<int, kBoardSquareCount>, kBoardSquareCount>, 2>;
using LowPlyHistoryTable = std::array<std::array<std::array<int, kBoardSquareCount>, kBoardSquareCount>, 8>;
using CaptureHistoryTable = std::array<std::array<std::array<int, 7>, kBoardSquareCount>, 13>;
using CounterMoveTable = std::array<std::array<Move, kBoardSquareCount>, 13>;
using ContinuationHistoryTable =
    std::array<std::array<std::array<std::array<int, kBoardSquareCount>, 13>, kBoardSquareCount>, 13>;
using ContinuationHistoryStack = std::array<ContinuationHistoryTable, 4>;

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
    void set_thread_count(int threads);
    [[nodiscard]] int thread_count() const;
    void set_multi_pv(int multi_pv);
    [[nodiscard]] int multi_pv() const;
    void set_move_overhead(std::chrono::milliseconds overhead);
    [[nodiscard]] std::chrono::milliseconds move_overhead() const;
    void set_slow_mover(int slow_mover);
    [[nodiscard]] int slow_mover() const;
    void stop();
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
    static constexpr int kCorrectionHistorySize = 16384;

    struct SearchStack {
        Move current_move;
        Piece moved_piece = Piece::None;
        Piece captured_piece = Piece::None;
        int static_eval = 0;
        int corrected_static_eval = 0;
        int stat_score = 0;
        bool has_static_eval = false;
        bool improving = false;
    };

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
    std::shared_ptr<nnue::Network> nnue_;
    SearchDiagnostics diagnostics_{};
    std::shared_ptr<TranspositionTable> tt_;
    std::shared_ptr<std::atomic_bool> stop_signal_;
    TimeManager time_manager_;
    int thread_count_ = 1;
    int multi_pv_ = 1;
    int worker_index_ = 0;
    std::array<std::array<Move, 2>, kMaxPly> killer_moves_{};
    HistoryTable history_{};
    LowPlyHistoryTable low_ply_history_{};
    CaptureHistoryTable capture_history_{};
    CounterMoveTable counter_moves_{};
    std::unique_ptr<ContinuationHistoryStack> continuation_history_;
    std::array<std::array<int, kCorrectionHistorySize>, 2> pawn_correction_history_{};
    std::array<std::array<int, kCorrectionHistorySize>, 2> minor_correction_history_{};
    std::array<std::array<int, kCorrectionHistorySize>, 2> nonpawn_white_correction_history_{};
    std::array<std::array<int, kCorrectionHistorySize>, 2> nonpawn_black_correction_history_{};
    std::vector<std::uint32_t> touched_history_;
    std::vector<std::uint32_t> touched_low_ply_history_;
    std::vector<std::uint32_t> touched_capture_history_;
    std::vector<std::uint32_t> touched_continuation_history_;
    std::vector<std::uint32_t> touched_correction_history_;
    std::array<SearchStack, kMaxPly> search_stack_{};
    std::array<nnue::QuantizedAccumulatorPair, kMaxPly> nnue_accumulator_stack_{};
    std::vector<Move> previous_iteration_pv_;
    std::vector<RootMoveInfo> root_moves_;
    MoveList root_search_moves_;
    bool root_search_moves_constrained_ = false;

    SearchResult search_single(Board& board, const SearchLimits& limits, bool prepare_shared_state);
    int root_search(Board& board, int depth, int alpha, int beta);
    void begin_root_iteration();
    void finish_root_iteration();
    void build_root_moves(const MoveList& legal_moves, const SearchLimits& limits);
    void sync_root_move_list();
    [[nodiscard]] RootMoveInfo* find_root_move(const Move& move);
    [[nodiscard]] const RootMoveInfo* best_root_move() const;
    int negamax(
        Board& board,
        int depth,
        int ply,
        int alpha,
        int beta,
        bool allow_null_move,
        int extensions_used,
        Move excluded_move = Move{}
    );
    int quiescence(Board& board, int ply, int alpha, int beta, int qply);
    int evaluate_with_diagnostics(const Board& board, int ply);
    int static_exchange_with_diagnostics(const Board& board, const Move& move);
    void refresh_nnue_accumulator(Board& board, int ply);
    void update_nnue_accumulator_after_move(const Board& board, const UndoState& undo, int parent_ply, int child_ply);
    void update_nnue_accumulator_after_null_move(int parent_ply, int child_ply);
    void age_history();
    void clear_history_touch_lists();
    [[nodiscard]] int corrected_static_eval(const Board& board, int raw_eval) const;
    void update_correction_history(const Board& board, Color side_to_move, int depth, int static_eval, int score);
    void merge_history_from(const Searcher& helper, int divisor);
    void record_cutoff(
        const Board& board,
        const Move& move,
        int depth,
        int ply,
        Color side_to_move,
        Piece moved_piece,
        Piece captured_piece,
        const Move* quiets_tried_before_cutoff,
        std::size_t quiet_count,
        const Move* captures_tried_before_cutoff,
        std::size_t capture_count
    );
    std::vector<Move> extract_principal_variation(Board& board, int depth) const;
    bool should_stop() const;
};

}  // namespace chess::engine
