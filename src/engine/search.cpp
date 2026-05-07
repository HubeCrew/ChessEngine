#include "chess/engine/search.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <future>
#include <limits>
#include <memory>
#include <new>
#include <thread>
#include <utility>
#include <vector>

#include "chess/core/attacks.h"
#include "chess/core/movegen.h"
#include "chess/engine/evaluation.h"
#include "chess/engine/static_exchange.h"
#include "chess/engine/tablebase.h"

namespace chess::engine {

bool move_gives_check(const Board& board, const Move& move) {
    return chess::move_gives_check(board, move);
}

namespace {

constexpr int kInfinity = 1'000'000;
constexpr int kMateScore = 900'000;
constexpr int kMateWindow = 1'000;
constexpr int kAspirationWindow = 50;
constexpr int kHistoryLimit = 1'000'000;
constexpr int kLmrMinDepth = 3;
constexpr int kLmrMoveIndex = 4;
constexpr int kNullMoveMinDepth = 4;
constexpr int kNullMoveMinMaterial = 500;
constexpr int kReverseFutilityMaxDepth = 7;
constexpr int kRazoringMaxDepth = 3;
constexpr int kProbCutMinDepth = 5;
constexpr int kProbCutMargin = 160;
constexpr int kFutilityMaxDepth = 5;
constexpr int kQuietSeePruneMaxDepth = 7;
constexpr int kCorrectionHistorySize = 16384;
constexpr int kMaxExtensionsPerLine = 6;
constexpr int kMaxQuiescenceQuietCheckPly = 1;
constexpr int kDeltaPruningMargin = 200;
constexpr std::size_t kMaxHistoryQuiets = 256;
constexpr std::size_t kMaxHistoryCaptures = 128;
constexpr std::size_t kNoMoveIndex = static_cast<std::size_t>(-1);
constexpr std::array<int, 4> kContinuationPlyOffsets{1, 2, 4, 6};
constexpr std::array<int, 4> kContinuationScoreWeights{100, 65, 40, 25};
constexpr std::array<int, 4> kContinuationUpdateWeights{128, 96, 64, 32};
constexpr int kGoodQuietScore = -12'000;

using ProfileClock = std::chrono::steady_clock;

std::uint64_t elapsed_ns_since(ProfileClock::time_point start) {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(ProfileClock::now() - start).count()
    );
}

void merge_nnue_profile(SearchDiagnostics& diagnostics, const nnue::ProfileCounters& profile) {
    diagnostics.nnue_refresh_placement_ns += profile.refresh_placement_ns;
    diagnostics.nnue_refresh_threat_ns += profile.refresh_threat_ns;
    diagnostics.nnue_update_placement_ns += profile.update_placement_ns;
    diagnostics.nnue_update_threat_ns += profile.update_threat_ns;
    diagnostics.nnue_dense_convert_ns += profile.dense_convert_ns;
    diagnostics.nnue_dense_forward_ns += profile.dense_forward_ns;
    diagnostics.nnue_dirty_squares += profile.dirty_squares;
    diagnostics.nnue_dirty_attackers_before += profile.dirty_attackers_before;
    diagnostics.nnue_dirty_attackers_after += profile.dirty_attackers_after;
    diagnostics.nnue_dirty_threat_removed += profile.dirty_threat_removed;
    diagnostics.nnue_dirty_threat_added += profile.dirty_threat_added;
    diagnostics.nnue_dirty_threat_unchanged += profile.dirty_threat_unchanged;
    diagnostics.nnue_fallback_parent_invalid += profile.fallback_parent_invalid;
    diagnostics.nnue_fallback_unsupported += profile.fallback_unsupported;
    diagnostics.nnue_fallback_invalid_move += profile.fallback_invalid_move;
    diagnostics.nnue_fallback_missing_king += profile.fallback_missing_king;
    diagnostics.nnue_fallback_moving_king += profile.fallback_moving_king;
    diagnostics.nnue_fallback_placement_feature += profile.fallback_placement_feature;
    diagnostics.nnue_fallback_dirty_overflow += profile.fallback_dirty_overflow;
    diagnostics.nnue_partial_refreshes += profile.partial_refreshes;
}

void add_search_diagnostics(SearchDiagnostics& lhs, const SearchDiagnostics& rhs) {
#define ADD_DIAGNOSTIC(field) lhs.field += rhs.field
    ADD_DIAGNOSTIC(evaluations);
    ADD_DIAGNOSTIC(evaluation_ns);
    ADD_DIAGNOSTIC(nnue_stack_evaluations);
    ADD_DIAGNOSTIC(nnue_stack_evaluation_ns);
    ADD_DIAGNOSTIC(classical_evaluations);
    ADD_DIAGNOSTIC(classical_evaluation_ns);
    ADD_DIAGNOSTIC(nnue_accumulator_refreshes);
    ADD_DIAGNOSTIC(nnue_accumulator_refresh_ns);
    ADD_DIAGNOSTIC(nnue_accumulator_update_attempts);
    ADD_DIAGNOSTIC(nnue_accumulator_update_successes);
    ADD_DIAGNOSTIC(nnue_accumulator_update_fallbacks);
    ADD_DIAGNOSTIC(nnue_accumulator_update_ns);
    ADD_DIAGNOSTIC(nnue_accumulator_null_copies);
    ADD_DIAGNOSTIC(nnue_refresh_placement_ns);
    ADD_DIAGNOSTIC(nnue_refresh_threat_ns);
    ADD_DIAGNOSTIC(nnue_update_placement_ns);
    ADD_DIAGNOSTIC(nnue_update_threat_ns);
    ADD_DIAGNOSTIC(nnue_dense_convert_ns);
    ADD_DIAGNOSTIC(nnue_dense_forward_ns);
    ADD_DIAGNOSTIC(nnue_dirty_squares);
    ADD_DIAGNOSTIC(nnue_dirty_attackers_before);
    ADD_DIAGNOSTIC(nnue_dirty_attackers_after);
    ADD_DIAGNOSTIC(nnue_dirty_threat_removed);
    ADD_DIAGNOSTIC(nnue_dirty_threat_added);
    ADD_DIAGNOSTIC(nnue_dirty_threat_unchanged);
    ADD_DIAGNOSTIC(nnue_fallback_parent_invalid);
    ADD_DIAGNOSTIC(nnue_fallback_unsupported);
    ADD_DIAGNOSTIC(nnue_fallback_invalid_move);
    ADD_DIAGNOSTIC(nnue_fallback_missing_king);
    ADD_DIAGNOSTIC(nnue_fallback_moving_king);
    ADD_DIAGNOSTIC(nnue_fallback_placement_feature);
    ADD_DIAGNOSTIC(nnue_fallback_dirty_overflow);
    ADD_DIAGNOSTIC(nnue_partial_refreshes);
    ADD_DIAGNOSTIC(move_picker_pv_picks);
    ADD_DIAGNOSTIC(move_picker_tt_picks);
    ADD_DIAGNOSTIC(move_picker_scored_moves);
    ADD_DIAGNOSTIC(move_picker_searched_moves);
    ADD_DIAGNOSTIC(move_picker_tactical_picks);
    ADD_DIAGNOSTIC(move_picker_killer_picks);
    ADD_DIAGNOSTIC(move_picker_quiet_picks);
    ADD_DIAGNOSTIC(beta_cutoffs);
    ADD_DIAGNOSTIC(beta_cutoff_move_index_sum);
    ADD_DIAGNOSTIC(illegal_pseudo_moves);
    ADD_DIAGNOSTIC(null_move_attempts);
    ADD_DIAGNOSTIC(null_move_cutoffs);
    ADD_DIAGNOSTIC(reverse_futility_prunes);
    ADD_DIAGNOSTIC(razoring_prunes);
    ADD_DIAGNOSTIC(probcut_attempts);
    ADD_DIAGNOSTIC(probcut_cutoffs);
    ADD_DIAGNOSTIC(futility_prunes);
    ADD_DIAGNOSTIC(late_move_prunes);
    ADD_DIAGNOSTIC(lmr_reductions);
    ADD_DIAGNOSTIC(lmr_researches);
    ADD_DIAGNOSTIC(singular_extension_attempts);
    ADD_DIAGNOSTIC(singular_extensions);
    ADD_DIAGNOSTIC(correction_history_updates);
    ADD_DIAGNOSTIC(tablebase_wdl_probes);
    ADD_DIAGNOSTIC(tablebase_wdl_hits);
    ADD_DIAGNOSTIC(tablebase_root_probes);
    ADD_DIAGNOSTIC(tablebase_root_hits);
    ADD_DIAGNOSTIC(book_probes);
    ADD_DIAGNOSTIC(book_hits);
    ADD_DIAGNOSTIC(qsearch_in_check_nodes);
    ADD_DIAGNOSTIC(qsearch_stand_pat_nodes);
    ADD_DIAGNOSTIC(see_calls);
    ADD_DIAGNOSTIC(move_gives_check_calls);
#undef ADD_DIAGNOSTIC
}

int color_index(Color color) {
    return static_cast<int>(color);
}

int piece_index(Piece piece) {
    return static_cast<int>(piece);
}

int captured_type_index(Piece piece) {
    return static_cast<int>(type_of(piece));
}

Piece captured_piece_for(const Board& board, const Move& move) {
    if ((move.flags & EnPassant) != 0) {
        return make_piece(opposite(board.side_to_move()), PieceType::Pawn);
    }
    return board.piece_at(move.to);
}

int mvv_lva_score(const Board& board, const Move& move) {
    const Piece attacker = board.piece_at(move.from);
    const Piece victim = captured_piece_for(board, move);
    if (victim == Piece::None) {
        return 0;
    }
    return 10 * material_value(type_of(victim)) - material_value(type_of(attacker));
}

int promotion_gain(const Move& move) {
    if (!move.is_promotion()) {
        return 0;
    }
    return material_value(move.promotion) - material_value(PieceType::Pawn);
}

int immediate_capture_gain(const Board& board, const Move& move) {
    return material_value(type_of(captured_piece_for(board, move))) + promotion_gain(move);
}

bool needs_see_for_loss_detection(const Board& board, const Move& move) {
    if (!move.is_capture() || move.is_promotion()) {
        return false;
    }

    const Piece attacker = board.piece_at(move.from);
    if (attacker == Piece::None) {
        return false;
    }

    return material_value(type_of(captured_piece_for(board, move))) < material_value(type_of(attacker));
}

bool capture_target_is_defended(const Board& board, const Move& move) {
    if (!move.is_capture() || (move.flags & EnPassant) != 0) {
        return false;
    }

    return attacks::attackers_to(board, move.to, opposite(board.side_to_move())) != 0;
}

int cheap_capture_exchange_score(const Board& board, const Move& move) {
    int score = immediate_capture_gain(board, move);
    if (needs_see_for_loss_detection(board, move) && capture_target_is_defended(board, move)) {
        const Piece attacker = board.piece_at(move.from);
        if (attacker != Piece::None) {
            score -= material_value(type_of(attacker));
        }
    }
    return score;
}

int score_to_tt(int score, int ply) {
    if (score > kMateScore - kMateWindow) {
        return score + ply;
    }
    if (score < -kMateScore + kMateWindow) {
        return score - ply;
    }
    return score;
}

int score_from_tt(int score, int ply) {
    if (score > kMateScore - kMateWindow) {
        return score - ply;
    }
    if (score < -kMateScore + kMateWindow) {
        return score + ply;
    }
    return score;
}

bool same_move(const Move& lhs, const Move& rhs) {
    return lhs.from == rhs.from
        && lhs.to == rhs.to
        && lhs.promotion == rhs.promotion
        && lhs.flags == rhs.flags;
}

bool same_move_identity(const Move& lhs, const Move& rhs) {
    return lhs.from == rhs.from
        && lhs.to == rhs.to
        && lhs.promotion == rhs.promotion;
}

bool contains_move_identity(const std::vector<Move>& moves, const Move& candidate) {
    return std::any_of(moves.begin(), moves.end(), [&](const Move& move) {
        return same_move_identity(move, candidate);
    });
}

bool is_valid_move_shape(const Move& move) {
    return is_valid_square(move.from) && is_valid_square(move.to);
}

bool is_quiet_history_move(const Move& move) {
    return !move.is_capture() && !move.is_promotion();
}

bool is_mate_score(int score) {
    return std::abs(score) > kMateScore - kMateWindow;
}

int non_pawn_material(const Board& board, Color side) {
    int total = 0;
    total += __builtin_popcountll(board.pieces(side, PieceType::Knight)) * material_value(PieceType::Knight);
    total += __builtin_popcountll(board.pieces(side, PieceType::Bishop)) * material_value(PieceType::Bishop);
    total += __builtin_popcountll(board.pieces(side, PieceType::Rook)) * material_value(PieceType::Rook);
    total += __builtin_popcountll(board.pieces(side, PieceType::Queen)) * material_value(PieceType::Queen);
    return total;
}

bool can_try_null_move(const Board& board, int depth, int ply, int alpha, int beta, bool in_check, bool allow_null_move) {
    const bool is_pv_node = beta - alpha > 1;
    return allow_null_move
        && !is_pv_node
        && !in_check
        && ply > 0
        && depth >= kNullMoveMinDepth
        && beta < kMateScore - kMateWindow
        && alpha > -kMateScore + kMateWindow
        && non_pawn_material(board, board.side_to_move()) >= kNullMoveMinMaterial;
}

bool move_is_legal_after_make(const Board& board, Color moving_side) {
    return !board.in_check(moving_side);
}

bool is_passed_pawn_after_move(const Board& board, Square square, Color color) {
    const Color enemy = opposite(color);
    const int direction = color == Color::White ? 1 : -1;
    for (int file = std::max(0, file_of(square) - 1); file <= std::min(7, file_of(square) + 1); ++file) {
        for (int rank = rank_of(square) + direction; rank >= 0 && rank < 8; rank += direction) {
            if (board.piece_at(make_square(file, rank)) == make_piece(enemy, PieceType::Pawn)) {
                return false;
            }
        }
    }
    return true;
}

bool is_advanced_passed_pawn_move(const Board& board_after_move, const Move& move, Color moving_side) {
    const Piece moved_piece = board_after_move.piece_at(move.to);
    if (moved_piece != make_piece(moving_side, PieceType::Pawn)) {
        return false;
    }

    const int advanced_rank = moving_side == Color::White ? rank_of(move.to) : 7 - rank_of(move.to);
    return advanced_rank >= 5 && is_passed_pawn_after_move(board_after_move, move.to, moving_side);
}

int extension_for_move(
    const Board& board_after_move,
    const Move& move,
    bool parent_in_check,
    bool /*gives_check*/,
    Color moving_side,
    int depth,
    int extensions_used
) {
    if (extensions_used >= kMaxExtensionsPerLine || depth <= 1) {
        return 0;
    }
    if (parent_in_check) {
        return 1;
    }
    if (is_advanced_passed_pawn_move(board_after_move, move, moving_side)) {
        return 1;
    }
    return 0;
}

int clamp_history(int value) {
    return std::clamp(value, -kHistoryLimit, kHistoryLimit);
}

void update_history_score(int& value, int bonus) {
    bonus = std::clamp(bonus, -kHistoryLimit / 2, kHistoryLimit / 2);
    value += bonus - value * std::abs(bonus) / kHistoryLimit;
    value = clamp_history(value);
}

int history_bonus(int depth) {
    return std::min(32'000, 16 * depth * depth + 128 * depth - 64);
}

int correction_bonus(int depth, int delta) {
    return std::clamp(delta * std::min(depth + 1, 16), -32'000, 32'000);
}

int pawn_correction_index(const Board& board) {
    const std::uint64_t pawns = board.pieces(Color::White, PieceType::Pawn)
        ^ (board.pieces(Color::Black, PieceType::Pawn) * 0x9e3779b97f4a7c15ULL);
    return static_cast<int>((pawns ^ (pawns >> 32)) & (kCorrectionHistorySize - 1));
}

int minor_correction_index(const Board& board) {
    const std::uint64_t minors =
        board.pieces(Color::White, PieceType::Knight)
        ^ (board.pieces(Color::White, PieceType::Bishop) << 1)
        ^ (board.pieces(Color::Black, PieceType::Knight) * 0x94d049bb133111ebULL)
        ^ (board.pieces(Color::Black, PieceType::Bishop) * 0xbf58476d1ce4e5b9ULL);
    return static_cast<int>((minors ^ (minors >> 32)) & (kCorrectionHistorySize - 1));
}

int nonpawn_correction_index(const Board& board, Color side) {
    const std::uint64_t rooks = board.pieces(side, PieceType::Rook);
    const std::uint64_t queens = board.pieces(side, PieceType::Queen);
    const std::uint64_t king = board.pieces(side, PieceType::King);
    const std::uint64_t signature = rooks ^ (queens << 1) ^ (king * 0x9e3779b97f4a7c15ULL);
    return static_cast<int>((signature ^ (signature >> 32)) & (kCorrectionHistorySize - 1));
}

int capture_history_bonus(int depth) {
    return std::min(24'000, 12 * depth * depth + 96 * depth);
}

int quiet_lmp_limit(int depth, bool improving) {
    const int base = improving ? 4 : 3;
    return base + depth * depth / (improving ? 2 : 3);
}

int reverse_futility_margin(int depth, bool improving) {
    return (improving ? 65 : 95) * depth - 25;
}

int futility_margin(int depth, bool improving) {
    return (improving ? 80 : 115) * depth + 80;
}

int razoring_margin(int depth) {
    return 220 + depth * 90;
}

int null_move_reduction(int depth, int static_eval, int beta) {
    int reduction = 3 + depth / 4;
    if (static_eval - beta > 200) {
        ++reduction;
    }
    return std::min(depth - 1, reduction);
}

int late_move_reduction(int depth, int move_index, int stat_score, bool improving, bool is_pv_node, bool cut_node) {
    if (depth < kLmrMinDepth || move_index < kLmrMoveIndex) {
        return 0;
    }

    int reduction = static_cast<int>(std::log(static_cast<double>(depth)) * std::log(static_cast<double>(move_index + 1)) / 1.80);
    if (!improving) {
        ++reduction;
    }
    if (cut_node) {
        ++reduction;
    }
    if (is_pv_node) {
        --reduction;
    }
    if (stat_score > 18'000) {
        reduction -= 2;
    } else if (stat_score > 6'000) {
        --reduction;
    } else if (stat_score < -18'000) {
        reduction += 2;
    } else if (stat_score < -6'000) {
        ++reduction;
    }
    return std::clamp(reduction, 0, depth - 1);
}

enum class MovePickerStage {
    PreviousPv,
    TtMove,
    GenerateCaptures,
    GoodCaptures,
    FirstKiller,
    SecondKiller,
    CounterMove,
    GenerateQuiets,
    GoodQuiets,
    BadCaptures,
    BadQuiets,
    GenerateEvasions,
    Evasions,
    Done,
};

enum class MovePickerMode {
    Main,
    Evasion,
    ProbCut,
    QSearch,
};

struct PickedMove {
    Move move;
    bool see_known = false;
    int see_score = 0;
};

struct ContinuationContext {
    Piece previous_piece = Piece::None;
    Square previous_to = kNoSquare;
    int weight = 0;
};

template <typename T, std::size_t Capacity>
class FixedVector {
public:
    FixedVector() = default;

    FixedVector(const FixedVector&) = delete;
    FixedVector& operator=(const FixedVector&) = delete;

    ~FixedVector() {
        clear();
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        assert(size_ < Capacity);
        T* item = std::construct_at(ptr(size_), std::forward<Args>(args)...);
        ++size_;
        return *item;
    }

    void clear() {
        for (std::size_t index = 0; index < size_; ++index) {
            std::destroy_at(ptr(index));
        }
        size_ = 0;
    }

    [[nodiscard]] std::size_t size() const {
        return size_;
    }

    [[nodiscard]] bool empty() const {
        return size_ == 0;
    }

    [[nodiscard]] T& operator[](std::size_t index) {
        assert(index < size_);
        return *ptr(index);
    }

    [[nodiscard]] const T& operator[](std::size_t index) const {
        assert(index < size_);
        return *ptr(index);
    }

    [[nodiscard]] T* begin() {
        return ptr(0);
    }

    [[nodiscard]] T* end() {
        return ptr(size_);
    }

private:
    alignas(T) std::byte storage_[sizeof(T) * Capacity];
    std::size_t size_ = 0;

    [[nodiscard]] T* ptr(std::size_t index) {
        return std::launder(reinterpret_cast<T*>(storage_ + sizeof(T) * index));
    }

    [[nodiscard]] const T* ptr(std::size_t index) const {
        return std::launder(reinterpret_cast<const T*>(storage_ + sizeof(T) * index));
    }
};

class MovePicker {
public:
    MovePicker(
        Board& board,
        const MoveList& moves,
        MovePickerMode mode,
        Move previous_pv_move,
        Move tt_move,
        Move first_killer,
        Move second_killer,
        Move counter_move,
        std::array<ContinuationContext, kContinuationPlyOffsets.size()> continuation_contexts,
        int ply,
        bool use_see_for_ordering,
        const HistoryTable& history,
        const LowPlyHistoryTable& low_ply_history,
        const CaptureHistoryTable& capture_history,
        const ContinuationHistoryStack& continuation_history,
        SearchDiagnostics& diagnostics
    )
        : board_(board),
          side_to_move_(board.side_to_move()),
          mode_(mode),
          previous_pv_move_(previous_pv_move),
          tt_move_(tt_move),
          first_killer_(first_killer),
          second_killer_(second_killer),
          counter_move_(counter_move),
          continuation_contexts_(continuation_contexts),
          ply_(ply),
          use_see_for_ordering_(use_see_for_ordering),
          history_(history),
          low_ply_history_(low_ply_history),
          capture_history_(capture_history),
          continuation_history_(continuation_history),
          diagnostics_(diagnostics) {
        for (const Move& move : moves) {
            moves_.emplace_back(move);
        }
    }

    bool next(PickedMove& picked) {
        while (stage_ != MovePickerStage::Done) {
            switch (stage_) {
                case MovePickerStage::PreviousPv:
                    if (mode_ == MovePickerMode::Main && pick_special(previous_pv_move_, picked)) {
                        ++diagnostics_.move_picker_pv_picks;
                        return true;
                    }
                    stage_ = MovePickerStage::TtMove;
                    break;
                case MovePickerStage::TtMove:
                    if (pick_special(tt_move_, picked)) {
                        ++diagnostics_.move_picker_tt_picks;
                        return true;
                    }
                    stage_ = mode_ == MovePickerMode::Evasion
                        ? MovePickerStage::GenerateEvasions
                        : MovePickerStage::GenerateCaptures;
                    break;
                case MovePickerStage::GenerateCaptures:
                    build_captures();
                    stage_ = MovePickerStage::GoodCaptures;
                    break;
                case MovePickerStage::GoodCaptures:
                    if (pick_from_ordered(good_captures_, good_capture_cursor_, picked)) {
                        return true;
                    }
                    if (mode_ == MovePickerMode::ProbCut) {
                        stage_ = MovePickerStage::Done;
                    } else if (mode_ == MovePickerMode::QSearch) {
                        stage_ = MovePickerStage::BadCaptures;
                    } else {
                        stage_ = MovePickerStage::FirstKiller;
                    }
                    break;
                case MovePickerStage::FirstKiller:
                    if (pick_quiet_special(first_killer_, picked)) {
                        ++diagnostics_.move_picker_killer_picks;
                        return true;
                    }
                    stage_ = MovePickerStage::SecondKiller;
                    break;
                case MovePickerStage::SecondKiller:
                    if (pick_quiet_special(second_killer_, picked)) {
                        ++diagnostics_.move_picker_killer_picks;
                        return true;
                    }
                    stage_ = MovePickerStage::CounterMove;
                    break;
                case MovePickerStage::CounterMove:
                    if (pick_quiet_special(counter_move_, picked)) {
                        return true;
                    }
                    stage_ = MovePickerStage::GenerateQuiets;
                    break;
                case MovePickerStage::GenerateQuiets:
                    build_quiets();
                    stage_ = MovePickerStage::GoodQuiets;
                    break;
                case MovePickerStage::GoodQuiets:
                    if (pick_from_ordered(good_quiets_, good_quiet_cursor_, picked)) {
                        return true;
                    }
                    stage_ = mode_ == MovePickerMode::QSearch
                        ? MovePickerStage::BadQuiets
                        : MovePickerStage::BadCaptures;
                    break;
                case MovePickerStage::BadCaptures:
                    if (pick_from_ordered(bad_captures_, bad_capture_cursor_, picked)) {
                        return true;
                    }
                    stage_ = mode_ == MovePickerMode::QSearch
                        ? MovePickerStage::GenerateQuiets
                        : MovePickerStage::BadQuiets;
                    break;
                case MovePickerStage::BadQuiets:
                    if (pick_from_ordered(bad_quiets_, bad_quiet_cursor_, picked)) {
                        return true;
                    }
                    stage_ = MovePickerStage::Done;
                    break;
                case MovePickerStage::GenerateEvasions:
                    build_evasions();
                    stage_ = MovePickerStage::Evasions;
                    break;
                case MovePickerStage::Evasions:
                    if (pick_from_ordered(evasions_, evasion_cursor_, picked)) {
                        return true;
                    }
                    stage_ = MovePickerStage::Done;
                    break;
                case MovePickerStage::Done:
                    break;
            }
        }
        return false;
    }

private:
    struct ScoredMove {
        Move move;
        bool emitted = false;
        bool tactical_classified = false;
        bool is_tactical = false;
        bool good_tactical = false;
        int tactical_score = 0;
        int tactical_exchange_score = 0;
        bool quiet_score_known = false;
        int quiet_score = 0;
        bool see_known = false;
        int see_score = 0;
    };

    struct ScoredIndex {
        ScoredIndex(std::size_t move_index, int move_score)
            : index(move_index),
              score(move_score) {}

        std::size_t index = kNoMoveIndex;
        int score = 0;
    };

    Board& board_;
    Color side_to_move_ = Color::White;
    MovePickerMode mode_ = MovePickerMode::Main;
    Move previous_pv_move_;
    Move tt_move_;
    Move first_killer_;
    Move second_killer_;
    Move counter_move_;
    std::array<ContinuationContext, kContinuationPlyOffsets.size()> continuation_contexts_{};
    int ply_ = 0;
    bool use_see_for_ordering_ = true;
    const HistoryTable& history_;
    const LowPlyHistoryTable& low_ply_history_;
    const CaptureHistoryTable& capture_history_;
    const ContinuationHistoryStack& continuation_history_;
    SearchDiagnostics& diagnostics_;
    FixedVector<ScoredMove, MoveList::kCapacity> moves_;
    FixedVector<ScoredIndex, MoveList::kCapacity> good_captures_;
    FixedVector<ScoredIndex, MoveList::kCapacity> bad_captures_;
    FixedVector<ScoredIndex, MoveList::kCapacity> good_quiets_;
    FixedVector<ScoredIndex, MoveList::kCapacity> bad_quiets_;
    FixedVector<ScoredIndex, MoveList::kCapacity> evasions_;
    MovePickerStage stage_ = MovePickerStage::PreviousPv;
    std::size_t good_capture_cursor_ = 0;
    std::size_t bad_capture_cursor_ = 0;
    std::size_t good_quiet_cursor_ = 0;
    std::size_t bad_quiet_cursor_ = 0;
    std::size_t evasion_cursor_ = 0;

    void build_captures() {
        for (std::size_t index = 0; index < moves_.size(); ++index) {
            ScoredMove& state = moves_[index];
            if (state.emitted) {
                continue;
            }
            classify_tactical(state);
            if (!state.is_tactical) {
                continue;
            }
            if (state.good_tactical) {
                good_captures_.emplace_back(index, state.tactical_score);
            } else {
                bad_captures_.emplace_back(index, state.tactical_score);
            }
        }
        sort_descending(good_captures_);
        sort_descending(bad_captures_);
    }

    void build_quiets() {
        for (std::size_t index = 0; index < moves_.size(); ++index) {
            ScoredMove& state = moves_[index];
            if (state.emitted) {
                continue;
            }
            classify_tactical(state);
            if (state.is_tactical) {
                continue;
            }
            const int score = quiet_score(state);
            if (score >= kGoodQuietScore) {
                good_quiets_.emplace_back(index, score);
            } else {
                bad_quiets_.emplace_back(index, score);
            }
        }
        sort_descending(good_quiets_);
        sort_descending(bad_quiets_);
    }

    void build_evasions() {
        for (std::size_t index = 0; index < moves_.size(); ++index) {
            ScoredMove& state = moves_[index];
            if (state.emitted) {
                continue;
            }
            classify_tactical(state);
            int score = 0;
            if (state.is_tactical) {
                score = state.tactical_score + 10'000;
            } else {
                score = quiet_score(state);
                if (same_move(state.move, first_killer_)) {
                    score += 30'000;
                } else if (same_move(state.move, second_killer_)) {
                    score += 29'000;
                } else if (same_move_identity(state.move, counter_move_)) {
                    score += 28'000;
                }
            }
            evasions_.emplace_back(index, score);
        }
        sort_descending(evasions_);
    }

    template <std::size_t Capacity>
    void sort_descending(FixedVector<ScoredIndex, Capacity>& indices) {
        std::stable_sort(indices.begin(), indices.end(), [](const ScoredIndex& lhs, const ScoredIndex& rhs) {
            return lhs.score > rhs.score;
        });
    }

    bool pick_special(const Move& move, PickedMove& picked) {
        if (!is_valid_move_shape(move)) {
            return false;
        }
        for (std::size_t index = 0; index < moves_.size(); ++index) {
            ScoredMove& state = moves_[index];
            if (state.emitted || !same_move_identity(state.move, move)) {
                continue;
            }
            emit(state, picked);
            return true;
        }
        return false;
    }

    bool pick_quiet_special(const Move& move, PickedMove& picked) {
        if (!is_valid_move_shape(move)) {
            return false;
        }
        for (std::size_t index = 0; index < moves_.size(); ++index) {
            ScoredMove& state = moves_[index];
            if (state.emitted || !same_move_identity(state.move, move)) {
                continue;
            }
            classify_tactical(state);
            if (state.is_tactical) {
                return false;
            }
            emit(state, picked);
            return true;
        }
        return false;
    }

    bool pick_from_ordered(
        FixedVector<ScoredIndex, MoveList::kCapacity>& indices,
        std::size_t& cursor,
        PickedMove& picked
    ) {
        while (cursor < indices.size()) {
            ScoredMove& state = moves_[indices[cursor].index];
            ++cursor;
            if (state.emitted) {
                continue;
            }
            emit(state, picked);
            return true;
        }
        return false;
    }

    void emit(ScoredMove& state, PickedMove& picked) {
        state.emitted = true;
        ++diagnostics_.move_picker_searched_moves;
        classify_tactical(state);
        if (state.is_tactical) {
            ++diagnostics_.move_picker_tactical_picks;
        } else {
            ++diagnostics_.move_picker_quiet_picks;
        }
        picked = PickedMove{
            state.move,
            state.see_known,
            state.see_score,
        };
    }

    void classify_tactical(ScoredMove& state) {
        if (state.tactical_classified) {
            return;
        }
        state.tactical_classified = true;
        ++diagnostics_.move_picker_scored_moves;
        state.is_tactical = state.move.is_capture() || state.move.is_promotion();
        if (!state.is_tactical) {
            return;
        }

        if (state.move.is_promotion()) {
            state.tactical_score += 90'000 + material_value(state.move.promotion);
            state.good_tactical = true;
        }

        if (state.move.is_capture()) {
            const bool see_needed = use_see_for_ordering_ && needs_see_for_loss_detection(board_, state.move);
            const int exchange_score = see_needed
                ? see(board_, state.move)
                : cheap_capture_exchange_score(board_, state.move);
            state.tactical_exchange_score = exchange_score;
            if (see_needed) {
                state.see_known = true;
                state.see_score = exchange_score;
            }

            const Piece moved_piece = board_.piece_at(state.move.from);
            const Piece captured_piece = captured_piece_for(board_, state.move);
            const int capture_history_score =
                moved_piece == Piece::None || captured_piece == Piece::None
                    ? 0
                    : capture_history_[piece_index(moved_piece)][state.move.to][captured_type_index(captured_piece)];
            if (exchange_score >= 0) {
                state.tactical_score += 70'000 + exchange_score + mvv_lva_score(board_, state.move);
                state.good_tactical = true;
            } else {
                state.tactical_score += 5'000 + exchange_score + mvv_lva_score(board_, state.move);
            }
            state.tactical_score += capture_history_score / 8;
        }
    }

    int quiet_score(ScoredMove& state) {
        if (!state.quiet_score_known) {
            state.quiet_score_known = true;
            const Piece moved_piece = board_.piece_at(state.move.from);
            state.quiet_score = history_[color_index(side_to_move_)][state.move.from][state.move.to];
            if (ply_ >= 0 && ply_ < static_cast<int>(low_ply_history_.size())) {
                state.quiet_score += low_ply_history_[static_cast<std::size_t>(ply_)][state.move.from][state.move.to];
            }
            if (moved_piece != Piece::None) {
                for (std::size_t index = 0; index < continuation_contexts_.size(); ++index) {
                    const ContinuationContext& context = continuation_contexts_[index];
                    if (context.previous_piece == Piece::None || !is_valid_square(context.previous_to)) {
                        continue;
                    }
                    const int continuation_score =
                        continuation_history_[index][piece_index(context.previous_piece)][context.previous_to]
                                             [piece_index(moved_piece)][state.move.to];
                    state.quiet_score += continuation_score * context.weight / 128;
                }
            }
        }
        return state.quiet_score;
    }

    int see(const Board& board, const Move& move) {
        ++diagnostics_.see_calls;
        return static_exchange_eval(board, move);
    }
};

void append_moves(MoveList& destination, const MoveList& source) {
    for (const Move& move : source) {
        destination.push_back(move);
    }
}

int quiet_history_score(
    const HistoryTable& history,
    const LowPlyHistoryTable& low_ply_history,
    const ContinuationHistoryStack& continuation_history,
    const std::array<ContinuationContext, kContinuationPlyOffsets.size()>& contexts,
    Color side_to_move,
    Piece moved_piece,
    const Move& move,
    int ply
) {
    int score = history[color_index(side_to_move)][move.from][move.to];
    if (ply >= 0 && ply < static_cast<int>(low_ply_history.size())) {
        score += low_ply_history[static_cast<std::size_t>(ply)][move.from][move.to];
    }
    if (moved_piece == Piece::None) {
        return score;
    }
    for (std::size_t index = 0; index < contexts.size(); ++index) {
        const ContinuationContext& context = contexts[index];
        if (context.previous_piece == Piece::None || !is_valid_square(context.previous_to)) {
            continue;
        }
        score += continuation_history[index][piece_index(context.previous_piece)][context.previous_to]
                                     [piece_index(moved_piece)][move.to]
            * context.weight / 128;
    }
    return score;
}

int capture_history_score(
    const CaptureHistoryTable& capture_history,
    Piece moved_piece,
    Piece captured_piece,
    const Move& move
) {
    if (moved_piece == Piece::None || captured_piece == Piece::None) {
        return 0;
    }
    return capture_history[piece_index(moved_piece)][move.to][captured_type_index(captured_piece)];
}

void blend_history_value(int& target, int source, int divisor) {
    if (target == source) {
        return;
    }
    target = clamp_history(target + (source - target) / std::max(1, divisor));
}

std::uint32_t encode_history_key(int color, Square from, Square to) {
    return static_cast<std::uint32_t>((color * kBoardSquareCount + from) * kBoardSquareCount + to);
}

std::uint32_t encode_low_ply_history_key(int ply, Square from, Square to) {
    return static_cast<std::uint32_t>((ply * kBoardSquareCount + from) * kBoardSquareCount + to);
}

std::uint32_t encode_capture_history_key(Piece moved_piece, Square to, Piece captured_piece) {
    return static_cast<std::uint32_t>((piece_index(moved_piece) * kBoardSquareCount + to) * 7 + captured_type_index(captured_piece));
}

std::uint32_t encode_continuation_history_key(
    int distance,
    Piece previous_piece,
    Square previous_to,
    Piece moved_piece,
    Square to
) {
    return static_cast<std::uint32_t>(
        ((((distance * 13 + piece_index(previous_piece)) * kBoardSquareCount + previous_to) * 13
             + piece_index(moved_piece))
            * kBoardSquareCount)
        + to
    );
}

std::uint32_t encode_correction_history_key(int kind, int color, int index) {
    return static_cast<std::uint32_t>((kind * 2 + color) * kCorrectionHistorySize + index);
}

template <typename Fn>
void for_each_unique_key(std::vector<std::uint32_t> keys, Fn&& fn) {
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    for (std::uint32_t key : keys) {
        fn(key);
    }
}

}  // namespace

void TimeManager::clear() {
    optimum_ = std::chrono::milliseconds{0};
    maximum_ = std::chrono::milliseconds{0};
    previous_time_reduction_ = 1.0;
}

void TimeManager::set_config(TimeManagementConfig config) {
    config.move_overhead = std::max(std::chrono::milliseconds{0}, config.move_overhead);
    config.slow_mover = std::clamp(config.slow_mover, 10, 1000);
    config_ = config;
}

TimeManagementConfig TimeManager::config() const {
    return config_;
}

void TimeManager::init(const Board& board, const SearchLimits& limits) {
    optimum_ = std::chrono::milliseconds{0};
    maximum_ = std::chrono::milliseconds{0};
    if (limits.move_time.count() > 0) {
        const auto overhead = std::min(config_.move_overhead, limits.move_time);
        optimum_ = std::max(std::chrono::milliseconds{1}, limits.move_time - overhead);
        maximum_ = optimum_;
        return;
    }

    const bool white_to_move = board.side_to_move() == Color::White;
    const auto remaining = white_to_move ? limits.white_time : limits.black_time;
    const auto increment = white_to_move ? limits.white_increment : limits.black_increment;
    if (remaining.count() <= 0) {
        return;
    }

    const auto overhead = std::min(config_.move_overhead, remaining);
    const auto available = std::max(std::chrono::milliseconds{1}, remaining - overhead);
    const double slow_mover_scale = static_cast<double>(config_.slow_mover) / 100.0;
    const int ply_estimate = std::max(0, board.fullmove_number() * 2 - (board.side_to_move() == Color::White ? 2 : 1));

    double optimum_scale = 0.0;
    double maximum_scale = 0.0;
    if (limits.moves_to_go > 0) {
        const double moves_to_go = static_cast<double>(std::clamp(limits.moves_to_go, 1, 120));
        optimum_scale = (0.88 + static_cast<double>(ply_estimate) / 116.4) / moves_to_go;
        maximum_scale = 1.3 + 0.11 * moves_to_go;
    } else {
        optimum_scale = std::pow(static_cast<double>(ply_estimate) + 2.94693, 0.461073) * 0.0120;
        maximum_scale = 5.5 + static_cast<double>(ply_estimate) / 12.0;
    }

    const std::int64_t available_ms = available.count();
    const std::int64_t increment_ms = increment.count();
    std::int64_t optimum_ms = static_cast<std::int64_t>(
        static_cast<double>(available_ms) * optimum_scale * slow_mover_scale
    );
    optimum_ms += increment_ms * 75 / 100;
    optimum_ms = std::clamp<std::int64_t>(optimum_ms, 1, std::max<std::int64_t>(1, available_ms * 825 / 1000));

    std::int64_t maximum_ms = static_cast<std::int64_t>(static_cast<double>(optimum_ms) * maximum_scale);
    maximum_ms = std::clamp<std::int64_t>(maximum_ms, optimum_ms, std::max<std::int64_t>(1, available_ms * 825 / 1000));

    optimum_ = std::chrono::milliseconds{optimum_ms};
    maximum_ = std::chrono::milliseconds{maximum_ms};
}

bool TimeManager::active() const {
    return maximum_.count() > 0;
}

std::chrono::milliseconds TimeManager::optimum() const {
    return optimum_;
}

std::chrono::milliseconds TimeManager::maximum() const {
    return maximum_;
}

std::chrono::steady_clock::time_point TimeManager::deadline(std::chrono::steady_clock::time_point start) const {
    return start + maximum_;
}

bool TimeManager::should_stop_after_iteration(
    std::chrono::milliseconds elapsed,
    int completed_depth,
    int best_move_stability,
    int best_move_changes,
    int previous_score,
    int best_score,
    int previous_average_score,
    std::uint64_t best_effort,
    std::uint64_t total_nodes
) {
    if (!active() || completed_depth <= 1) {
        return false;
    }
    if (elapsed >= maximum_) {
        return true;
    }

    const double nodes_effort = total_nodes == 0
        ? 0.0
        : static_cast<double>(best_effort) / static_cast<double>(total_nodes);
    const double falling_eval_raw =
        (12.44 + 2.318 * static_cast<double>(previous_average_score - best_score)
            + 0.95 * static_cast<double>(previous_score - best_score)) / 100.0;
    const double falling_eval = std::clamp(falling_eval_raw, 0.581, 1.655);
    const double stability_center = static_cast<double>(best_move_stability) + 11.565;
    const double time_reduction = 0.64 + 0.93 / (0.953 + std::exp(-0.476 * (static_cast<double>(completed_depth) - stability_center)));
    const double reduction = (1.5 + previous_time_reduction_) / (2.255 * time_reduction);
    double effort_factor = 1.0;
    if (nodes_effort > 0.60) {
        effort_factor -= std::min(0.25, (nodes_effort - 0.60) * 0.60);
    } else if (nodes_effort < 0.35) {
        effort_factor += std::min(0.25, (0.35 - nodes_effort) * 0.75);
    }
    const double change_factor = 1.0 + std::min(0.55, 0.14 * static_cast<double>(best_move_changes));

    const double scale = std::clamp(falling_eval * reduction * effort_factor * change_factor, 0.55, 1.90);
    const auto adjusted_optimum = std::chrono::milliseconds{
        std::clamp<std::int64_t>(
            static_cast<std::int64_t>(static_cast<double>(optimum_.count()) * scale),
            std::max<std::int64_t>(1, optimum_.count() / 2),
            maximum_.count()
        )
    };
    const bool stop = elapsed >= adjusted_optimum;
    if (stop) {
        previous_time_reduction_ = time_reduction;
    }
    return stop;
}

Searcher::Searcher() {
    nnue_ = std::make_shared<nnue::Network>();
    tt_ = std::make_shared<TranspositionTable>();
    stop_signal_ = std::make_shared<std::atomic_bool>(false);
    opening_book_ = std::make_shared<book::OpeningBook>();
    continuation_history_ = std::make_unique<ContinuationHistoryStack>();
    clear();
}

SearchResult Searcher::search(Board& board, const SearchLimits& limits) {
    stop_signal_->store(false, std::memory_order_relaxed);

    const int game_ply = std::max(0, (board.fullmove_number() - 1) * 2 + (board.side_to_move() == Color::Black ? 1 : 0));
    if (own_book_ && opening_book_->loaded() && book_depth_ > 0 && game_ply < book_depth_) {
        SearchDiagnostics book_diagnostics;
        ++book_diagnostics.book_probes;
        std::optional<book::ProbeMove> book_move = opening_book_->probe(
            board,
            limits.search_moves,
            best_book_move_,
            static_cast<std::uint16_t>(book_minimum_weight_)
        );
        if (book_move) {
            ++book_diagnostics.book_hits;
            SearchResult result;
            result.best_move = book_move->move;
            result.depth = 0;
            result.nodes = 0;
            result.qnodes = 0;
            result.score_centipawns = 0;
            result.elapsed = std::chrono::milliseconds{0};
            result.principal_variation = {book_move->move};
            result.lines.push_back(SearchResult::Line{1, book_move->move, 0, {book_move->move}});
            result.diagnostics = book_diagnostics;
            return result;
        }
    }

    tt_->new_search();
    age_history();

    const int workers = std::clamp(thread_count_, 1, 512);
    if (workers == 1 || !limits.search_moves.empty()) {
        return search_single(board, limits, false);
    }

    struct HelperSearch {
        std::shared_ptr<Searcher> helper;
        std::future<SearchResult> future;
    };

    std::vector<HelperSearch> futures;
    futures.reserve(static_cast<std::size_t>(workers - 1));
    for (int worker = 1; worker < workers; ++worker) {
        Board worker_board = board;
        auto helper = std::make_shared<Searcher>();
        helper->nnue_ = nnue_;
        helper->tt_ = tt_;
        helper->stop_signal_ = stop_signal_;
        helper->null_move_pruning_ = null_move_pruning_;
        helper->search_extensions_ = search_extensions_;
        helper->profiling_ = profiling_;
        helper->eval_type_ = eval_type_;
        helper->time_manager_.set_config(time_manager_.config());
        helper->thread_count_ = 1;
        helper->multi_pv_ = multi_pv_;
        // Fathom owns process-global probing state. Keep Syzygy probing on the
        // main worker only; helper threads still benefit from root/TT results
        // without contending on tablebase state during lazy SMP search.
        helper->syzygy_probe_depth_ = kMaxPly;
        helper->own_book_ = false;
        helper->best_book_move_ = best_book_move_;
        helper->book_depth_ = book_depth_;
        helper->book_minimum_weight_ = book_minimum_weight_;
        helper->opening_book_ = opening_book_;
        helper->worker_index_ = worker;
        helper->history_ = history_;
        helper->low_ply_history_ = low_ply_history_;
        helper->capture_history_ = capture_history_;
        helper->counter_moves_ = counter_moves_;
        *helper->continuation_history_ = *continuation_history_;
        helper->pawn_correction_history_ = pawn_correction_history_;
        helper->minor_correction_history_ = minor_correction_history_;
        helper->nonpawn_white_correction_history_ = nonpawn_white_correction_history_;
        helper->nonpawn_black_correction_history_ = nonpawn_black_correction_history_;
        futures.push_back(HelperSearch{
            helper,
            std::async(std::launch::async, [helper, worker_board, limits]() mutable {
                return helper->search_single(worker_board, limits, false);
            })
        });
    }

    SearchResult best = search_single(board, limits, false);
    SearchDiagnostics total_diagnostics = best.diagnostics;
    for (auto& helper_search : futures) {
        SearchResult candidate = helper_search.future.get();
        merge_history_from(*helper_search.helper, workers);
        add_search_diagnostics(total_diagnostics, candidate.diagnostics);
        const bool candidate_is_better = is_valid_move_shape(candidate.best_move)
            && (candidate.depth > best.depth
                || (candidate.depth == best.depth && candidate.nodes > best.nodes));
        const std::uint64_t previous_best_nodes = best.nodes;
        const std::uint64_t previous_best_qnodes = best.qnodes;
        const std::uint64_t previous_best_tt_hits = best.tt_hits;
        best.nodes += candidate.nodes;
        best.qnodes += candidate.qnodes;
        best.tt_hits += candidate.tt_hits;
        if (candidate_is_better) {
            const std::uint64_t total_nodes = previous_best_nodes + candidate.nodes;
            const std::uint64_t total_qnodes = previous_best_qnodes + candidate.qnodes;
            const std::uint64_t total_tt_hits = previous_best_tt_hits + candidate.tt_hits;
            candidate.nodes = total_nodes;
            candidate.qnodes = total_qnodes;
            candidate.tt_hits = total_tt_hits;
            best = std::move(candidate);
        }
    }
    best.diagnostics = total_diagnostics;

    if (best.elapsed.count() <= 0) {
        best.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_
        );
    }
    const auto elapsed_ms = std::max<std::int64_t>(1, best.elapsed.count());
    best.nps = best.nodes * 1000ULL / static_cast<std::uint64_t>(elapsed_ms);
    return best;
}

SearchResult Searcher::search_single(Board& board, const SearchLimits& limits, bool prepare_shared_state) {
    nodes_ = 0;
    qnodes_ = 0;
    tt_hits_ = 0;
    diagnostics_ = {};
    clear_history_touch_lists();
    if (prepare_shared_state) {
        stop_signal_->store(false, std::memory_order_relaxed);
        tt_->new_search();
        age_history();
    }
    time_manager_.init(board, limits);
    use_deadline_ = time_manager_.active();
    start_time_ = std::chrono::steady_clock::now();
    deadline_ = time_manager_.deadline(start_time_);
    previous_iteration_pv_.clear();
    refresh_nnue_accumulator(board, 0);

    SearchResult result;
    const MoveList legal_moves = generate_legal_moves(board);
    build_root_moves(legal_moves, limits);

    if (root_search_moves_.empty()) {
        result.best_move = Move{};
        result.score_centipawns = legal_moves.empty() && board.in_check(board.side_to_move()) ? -kMateScore : 0;
        result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_
        );
        return result;
    }

    const int max_depth = std::min(std::max(1, limits.depth), kMaxPly - 1);
    const bool main_worker = worker_index_ == 0;
    const std::optional<tablebase::RootProbeResult> tb_root = main_worker
        ? tablebase::probe_root(board)
        : std::nullopt;
    if (tb_root) {
        ++diagnostics_.tablebase_root_probes;
        ++diagnostics_.tablebase_root_hits;
        result.best_move = tb_root->best_move;
        result.score_centipawns = tb_root->score;
        result.depth = 1;
        result.nodes = nodes_;
        result.qnodes = qnodes_;
        result.tt_hits = tt_hits_;
        result.principal_variation = {result.best_move};
        result.lines.clear();
        const int pv_count = std::min<int>(std::max(1, multi_pv_), static_cast<int>(tb_root->moves.size()));
        for (int index = 0; index < pv_count; ++index) {
            const tablebase::RootMove& tb_move = tb_root->moves[static_cast<std::size_t>(index)];
            result.lines.push_back(SearchResult::Line{
                index + 1,
                tb_move.move,
                tb_move.score,
                {tb_move.move},
            });
        }
        result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_
        );
        result.diagnostics = diagnostics_;
        return result;
    } else if (main_worker && tablebase::initialized() && tablebase::probeable(board, true)) {
        ++diagnostics_.tablebase_root_probes;
    }

    int stable_best_move_iterations = 0;
    int best_move_changes = 0;
    for (int depth = 1; depth <= max_depth; ++depth) {
        if (should_stop()) {
            break;
        }

        previous_iteration_pv_ = result.principal_variation;
        begin_root_iteration();
        const Move previous_best_move = result.best_move;
        const int previous_score = result.score_centipawns;

        int alpha = -kInfinity;
        int beta = kInfinity;
        int window = kAspirationWindow;
        if (multi_pv_ == 1 && depth > 1 && !is_mate_score(result.score_centipawns)) {
            alpha = std::max(-kInfinity, result.score_centipawns - window);
            beta = std::min(kInfinity, result.score_centipawns + window);
        }

        int score = 0;
        while (true) {
            score = root_search(board, depth, alpha, beta);
            if (should_stop() || (score > alpha && score < beta)) {
                break;
            }

            if (alpha == -kInfinity && beta == kInfinity) {
                break;
            }

            window *= 2;
            if (window >= kInfinity / 2 || is_mate_score(score)) {
                alpha = -kInfinity;
                beta = kInfinity;
            } else {
                alpha = std::max(-kInfinity, score - window);
                beta = std::min(kInfinity, score + window);
            }
        }

        if (!should_stop()) {
            finish_root_iteration();
            const RootMoveInfo* best_root = best_root_move();
            result.best_move = best_root != nullptr ? best_root->move : root_search_moves_.front();
            result.score_centipawns = best_root != nullptr ? best_root->score : score;
            result.depth = depth;
            result.nodes = nodes_;
            result.qnodes = qnodes_;
            result.tt_hits = tt_hits_;
            result.diagnostics = diagnostics_;
            result.principal_variation = best_root != nullptr ? best_root->principal_variation : extract_principal_variation(board, depth);
            if (result.principal_variation.empty()) {
                result.principal_variation.push_back(result.best_move);
            }
            result.lines.clear();
            const int pv_count = std::min<int>(std::max(1, multi_pv_), static_cast<int>(root_moves_.size()));
            for (int index = 0; index < pv_count; ++index) {
                const RootMoveInfo& root = root_moves_[static_cast<std::size_t>(index)];
                if (!root.searched) {
                    continue;
                }
                result.lines.push_back(SearchResult::Line{
                    index + 1,
                    root.move,
                    root.score,
                    root.principal_variation,
                });
            }
            previous_iteration_pv_ = result.principal_variation;
            if (same_move_identity(result.best_move, previous_best_move)) {
                ++stable_best_move_iterations;
            } else {
                stable_best_move_iterations = 0;
                if (is_valid_move_shape(previous_best_move)) {
                    ++best_move_changes;
                }
            }
            const int previous_average_score = best_root != nullptr ? best_root->average_score : previous_score;
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time_
            );
            const std::uint64_t best_effort = best_root != nullptr ? best_root->effort : 0;
            if (time_manager_.should_stop_after_iteration(
                    elapsed,
                    depth,
                    stable_best_move_iterations,
                    best_move_changes,
                    previous_score,
                    result.score_centipawns,
                    previous_average_score,
                    best_effort,
                    nodes_)) {
                stop_signal_->store(true, std::memory_order_relaxed);
                break;
            }
        }
    }

    if (!is_valid_move_shape(result.best_move)) {
        result.best_move = root_search_moves_.front();
    }
    if (result.depth == 0 && is_valid_move_shape(result.best_move)) {
        result.depth = 1;
        result.score_centipawns = evaluate_with_diagnostics(board, 0);
        result.nodes = nodes_;
        result.qnodes = qnodes_;
        result.tt_hits = tt_hits_;
        result.principal_variation.clear();
        result.principal_variation.push_back(result.best_move);
    }

    result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time_
    );
    const auto elapsed_ms = std::max<std::int64_t>(1, result.elapsed.count());
    result.nps = result.nodes * 1000ULL / static_cast<std::uint64_t>(elapsed_ms);
    result.diagnostics = diagnostics_;
    return result;
}

void Searcher::set_hash_size_mb(std::size_t megabytes) {
    tt_->resize_mb(megabytes);
}

std::size_t Searcher::hash_size_mb() const {
    return tt_->size_mb();
}

void Searcher::set_null_move_pruning(bool enabled) {
    null_move_pruning_ = enabled;
}

bool Searcher::null_move_pruning() const {
    return null_move_pruning_;
}

void Searcher::set_search_extensions(bool enabled) {
    search_extensions_ = enabled;
}

bool Searcher::search_extensions() const {
    return search_extensions_;
}

void Searcher::set_profiling(bool enabled) {
    profiling_ = enabled;
}

bool Searcher::profiling() const {
    return profiling_;
}

void Searcher::set_thread_count(int threads) {
    thread_count_ = std::clamp(threads, 1, 512);
}

int Searcher::thread_count() const {
    return thread_count_;
}

void Searcher::set_multi_pv(int multi_pv) {
    multi_pv_ = std::clamp(multi_pv, 1, 256);
}

int Searcher::multi_pv() const {
    return multi_pv_;
}

bool Searcher::set_syzygy_path(const std::filesystem::path& path, std::string* error) {
    return tablebase::initialize(path, error);
}

std::filesystem::path Searcher::syzygy_path() const {
    return tablebase::path();
}

unsigned Searcher::syzygy_largest() const {
    return tablebase::largest();
}

void Searcher::set_syzygy_probe_depth(int depth) {
    syzygy_probe_depth_ = std::clamp(depth, 0, kMaxPly - 1);
}

int Searcher::syzygy_probe_depth() const {
    return syzygy_probe_depth_;
}

void Searcher::set_own_book(bool enabled) {
    own_book_ = enabled;
}

bool Searcher::own_book() const {
    return own_book_;
}

bool Searcher::load_book(const std::filesystem::path& path, std::string* error) {
    return opening_book_->load(path, error);
}

void Searcher::clear_book() {
    opening_book_->clear();
}

bool Searcher::book_loaded() const {
    return opening_book_->loaded();
}

std::filesystem::path Searcher::book_path() const {
    return opening_book_->path();
}

std::size_t Searcher::book_entry_count() const {
    return opening_book_->entry_count();
}

void Searcher::set_best_book_move(bool enabled) {
    best_book_move_ = enabled;
}

bool Searcher::best_book_move() const {
    return best_book_move_;
}

void Searcher::set_book_depth(int depth) {
    book_depth_ = std::clamp(depth, 0, 255);
}

int Searcher::book_depth() const {
    return book_depth_;
}

void Searcher::set_book_minimum_weight(int weight) {
    book_minimum_weight_ = std::clamp(weight, 0, static_cast<int>(std::numeric_limits<std::uint16_t>::max()));
}

int Searcher::book_minimum_weight() const {
    return book_minimum_weight_;
}

void Searcher::stop() {
    stop_signal_->store(true, std::memory_order_relaxed);
}

void Searcher::set_eval_type(EvalType type) {
    eval_type_ = type;
}

EvalType Searcher::eval_type() const {
    return eval_type_;
}

bool Searcher::load_nnue(const std::filesystem::path& path, std::string* error) {
    return nnue_->load(path, error);
}

void Searcher::clear_nnue() {
    nnue_->clear();
}

bool Searcher::nnue_loaded() const {
    return nnue_->loaded();
}

const nnue::ModelInfo& Searcher::nnue_info() const {
    return nnue_->info();
}

int Searcher::evaluate_nnue_white_perspective(const Board& board) const {
    if (!nnue_->loaded()) {
        return chess::engine::evaluate_white_perspective(board);
    }
    return nnue_->evaluate_white_perspective(board);
}

int Searcher::evaluate_white_perspective(const Board& board) const {
    return chess::engine::evaluate_white_perspective(board, EvaluationConfig{eval_type_, nnue_.get(), 50});
}

int Searcher::evaluate_side_to_move(const Board& board) const {
    return evaluate(board, EvaluationConfig{eval_type_, nnue_.get(), 50});
}

void Searcher::set_move_overhead(std::chrono::milliseconds overhead) {
    TimeManagementConfig config = time_manager_.config();
    config.move_overhead = overhead;
    time_manager_.set_config(config);
}

std::chrono::milliseconds Searcher::move_overhead() const {
    return time_manager_.config().move_overhead;
}

void Searcher::set_slow_mover(int slow_mover) {
    TimeManagementConfig config = time_manager_.config();
    config.slow_mover = slow_mover;
    time_manager_.set_config(config);
}

int Searcher::slow_mover() const {
    return time_manager_.config().slow_mover;
}

void Searcher::clear() {
    tt_->clear();
    time_manager_.clear();
    for (auto& ply_killers : killer_moves_) {
        ply_killers = {Move{}, Move{}};
    }
    for (auto& color_history : history_) {
        for (auto& from_history : color_history) {
            from_history.fill(0);
        }
    }
    for (auto& ply_history : low_ply_history_) {
        for (auto& from_history : ply_history) {
            from_history.fill(0);
        }
    }
    for (auto& piece_history : capture_history_) {
        for (auto& to_history : piece_history) {
            to_history.fill(0);
        }
    }
    for (auto& piece_counters : counter_moves_) {
        piece_counters.fill(Move{});
    }
    for (auto& distance_history : *continuation_history_) {
        for (auto& previous_piece_history : distance_history) {
            for (auto& previous_to_history : previous_piece_history) {
                for (auto& moved_piece_history : previous_to_history) {
                    moved_piece_history.fill(0);
                }
            }
        }
    }
    for (auto& color_correction : pawn_correction_history_) {
        color_correction.fill(0);
    }
    for (auto& color_correction : minor_correction_history_) {
        color_correction.fill(0);
    }
    for (auto& color_correction : nonpawn_white_correction_history_) {
        color_correction.fill(0);
    }
    for (auto& color_correction : nonpawn_black_correction_history_) {
        color_correction.fill(0);
    }
    for (auto& stack : search_stack_) {
        stack = SearchStack{};
    }
    previous_iteration_pv_.clear();
    root_moves_.clear();
    root_search_moves_.clear();
    clear_history_touch_lists();
}

void Searcher::build_root_moves(const MoveList& legal_moves, const SearchLimits& limits) {
    root_search_moves_constrained_ = !limits.search_moves.empty();
    root_moves_.clear();
    root_search_moves_.clear();

    for (const Move& move : legal_moves) {
        if (root_search_moves_constrained_ && !contains_move_identity(limits.search_moves, move)) {
            continue;
        }
        RootMoveInfo info;
        info.move = move;
        info.principal_variation.push_back(move);
        root_moves_.push_back(std::move(info));
    }

    if (worker_index_ > 0 && root_moves_.size() > 1) {
        const auto offset = static_cast<std::ptrdiff_t>(
            static_cast<std::size_t>(worker_index_) % root_moves_.size()
        );
        std::rotate(root_moves_.begin(), root_moves_.begin() + offset, root_moves_.end());
    }
    sync_root_move_list();
}

void Searcher::sync_root_move_list() {
    root_search_moves_.clear();
    for (const RootMoveInfo& root_move : root_moves_) {
        root_search_moves_.push_back(root_move.move);
    }
}

RootMoveInfo* Searcher::find_root_move(const Move& move) {
    const auto found = std::find_if(root_moves_.begin(), root_moves_.end(), [&](const RootMoveInfo& root_move) {
        return same_move_identity(root_move.move, move);
    });
    return found == root_moves_.end() ? nullptr : &*found;
}

const RootMoveInfo* Searcher::best_root_move() const {
    if (root_moves_.empty()) {
        return nullptr;
    }
    return &root_moves_.front();
}

void Searcher::begin_root_iteration() {
    for (RootMoveInfo& root_move : root_moves_) {
        root_move.previous_score = root_move.score;
        root_move.effort = 0;
        root_move.searched = false;
    }
}

void Searcher::finish_root_iteration() {
    for (RootMoveInfo& root_move : root_moves_) {
        if (!root_move.searched) {
            continue;
        }
        root_move.average_score = root_move.average_score == 0
            ? root_move.score
            : (root_move.average_score * 3 + root_move.score) / 4;
    }
    std::stable_sort(root_moves_.begin(), root_moves_.end(), [](const RootMoveInfo& lhs, const RootMoveInfo& rhs) {
        if (lhs.searched != rhs.searched) {
            return lhs.searched;
        }
        if (lhs.score != rhs.score) {
            return lhs.score > rhs.score;
        }
        return lhs.effort > rhs.effort;
    });
    sync_root_move_list();
}

int Searcher::root_search(Board& board, int depth, int alpha, int beta) {
    if (root_moves_.empty()) {
        return board.in_check(board.side_to_move()) ? -kMateScore : 0;
    }
    if (should_stop()) {
        return evaluate_with_diagnostics(board, 0);
    }

    ++nodes_;
    const int original_alpha = alpha;
    const int original_beta = beta;
    const bool multi_pv_root = multi_pv_ > 1;
    const bool in_check = board.in_check(board.side_to_move());
    Move best_move;
    int best_score = -kInfinity;
    int searched_moves = 0;

    for (RootMoveInfo& root_move : root_moves_) {
        const Move move = root_move.move;
        const std::uint64_t start_nodes = nodes_;
        const Color moving_side = board.side_to_move();
        const Piece moved_piece = board.piece_at(move.from);
        const Piece captured_piece = captured_piece_for(board, move);
        const UndoState undo = board.make_move(move);
        if (!move_is_legal_after_make(board, moving_side)) {
            ++diagnostics_.illegal_pseudo_moves;
            board.unmake_move(undo);
            continue;
        }

        update_nnue_accumulator_after_move(board, undo, 0, 1);
        const bool gives_check = board.in_check(board.side_to_move());
        const int extension = search_extensions_
            ? extension_for_move(board, move, in_check, gives_check, moving_side, depth, 0)
            : 0;
        const int child_depth = std::max(0, depth - 1 + extension);
        search_stack_[0].current_move = move;
        search_stack_[0].moved_piece = moved_piece;
        search_stack_[0].captured_piece = captured_piece;
        search_stack_[0].stat_score = is_quiet_history_move(move)
            ? quiet_history_score(
                history_,
                low_ply_history_,
                *continuation_history_,
                {},
                moving_side,
                moved_piece,
                move,
                0)
            : capture_history_score(capture_history_, moved_piece, captured_piece, move) / 8;

        const int alpha_before_move = alpha;
        int score = 0;
        if (multi_pv_root) {
            score = -negamax(board, child_depth, 1, -kInfinity, kInfinity, true, extension);
        } else if (searched_moves == 0) {
            score = -negamax(board, child_depth, 1, -beta, -alpha, true, extension);
        } else {
            score = -negamax(board, child_depth, 1, -alpha - 1, -alpha, true, extension);
            if (score > alpha && score < beta && !should_stop()) {
                score = -negamax(board, child_depth, 1, -beta, -alpha, true, extension);
            }
        }

        std::vector<Move> child_pv;
        if (!should_stop()) {
            child_pv = extract_principal_variation(board, child_depth);
        }
        board.unmake_move(undo);
        search_stack_[0].current_move = Move{};
        search_stack_[0].moved_piece = Piece::None;
        search_stack_[0].captured_piece = Piece::None;

        const std::uint64_t effort = nodes_ - start_nodes;
        root_move.effort += effort;
        root_move.nodes += effort;
        root_move.searched = true;
        root_move.score = searched_moves > 0 && score <= alpha_before_move
            ? std::max(-kInfinity, alpha_before_move - 1)
            : score;
        root_move.principal_variation.clear();
        root_move.principal_variation.push_back(move);
        root_move.principal_variation.insert(
            root_move.principal_variation.end(),
            child_pv.begin(),
            child_pv.end()
        );

        ++searched_moves;
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        if (!multi_pv_root) {
            alpha = std::max(alpha, score);
        }
        if ((!multi_pv_root && alpha >= beta) || should_stop()) {
            break;
        }
    }

    if (searched_moves == 0) {
        return in_check ? -kMateScore : 0;
    }

    Bound bound = Bound::Exact;
    if (best_score <= original_alpha) {
        bound = Bound::Upper;
    } else if (best_score >= original_beta) {
        bound = Bound::Lower;
    }
    tt_->store(board.hash_key(), depth, score_to_tt(best_score, 0), bound, best_move, kNoTranspositionStaticEval);
    return best_score;
}

int Searcher::negamax(
    Board& board,
    int depth,
    int ply,
    int alpha,
    int beta,
    bool allow_null_move,
    int extensions_used,
    Move excluded_move
) {
    if (should_stop()) {
        return evaluate_with_diagnostics(board, ply);
    }
    ++nodes_;

    const int original_alpha = alpha;
    const int original_beta = beta;
    assert(ply > 0);
    const bool is_pv_node = beta - alpha > 1;
    const bool is_excluded_node = is_valid_move_shape(excluded_move);
    const bool cut_node = !is_pv_node;
    SearchStack& stack = search_stack_[ply];
    stack = SearchStack{};

    if (depth == 0 || ply >= kMaxPly - 1) {
        return quiescence(board, ply, alpha, beta, 0);
    }

    const bool in_check = board.in_check(board.side_to_move());
    if (!is_pv_node
        && !is_excluded_node
        && !in_check
        && depth >= syzygy_probe_depth_
        && tablebase::initialized()
        && tablebase::probeable(board, false)) {
        ++diagnostics_.tablebase_wdl_probes;
        if (std::optional<tablebase::ProbeResult> tb = tablebase::probe_wdl(board)) {
            ++diagnostics_.tablebase_wdl_hits;
            return tb->score;
        }
    }
    Move tt_move;
    int tt_score = 0;
    int tt_static_eval = kNoTranspositionStaticEval;
    Bound tt_bound = Bound::Exact;
    int tt_depth = -1;
    const bool use_tt_entry = !is_excluded_node;
    if (use_tt_entry) {
        const std::optional<TranspositionEntry> entry = tt_->probe_entry(board.hash_key());
        if (entry.has_value()) {
            tt_move = entry->best_move;
            tt_depth = entry->depth;
            tt_score = score_from_tt(entry->score, ply);
            tt_bound = entry->bound;
            tt_static_eval = entry->static_eval;
            if (entry->depth >= depth) {
                ++tt_hits_;
                if (!is_pv_node) {
                    const bool tt_has_move = is_valid_move_shape(entry->best_move);
                    if (entry->bound == Bound::Exact) {
                        return tt_score;
                    }
                    if (entry->bound == Bound::Lower
                        && tt_score >= beta
                        && (tt_has_move || entry->depth >= depth + 2)) {
                        return tt_score;
                    }
                    if (entry->bound == Bound::Upper && tt_score <= alpha) {
                        return tt_score;
                    }
                }
            }
        }
    }

    int raw_static_eval = kNoTranspositionStaticEval;
    int static_eval = kNoTranspositionStaticEval;
    if (!in_check) {
        raw_static_eval = tt_static_eval != kNoTranspositionStaticEval
            ? tt_static_eval
            : evaluate_with_diagnostics(board, ply);
        static_eval = corrected_static_eval(board, raw_static_eval);
        stack.static_eval = raw_static_eval;
        stack.corrected_static_eval = static_eval;
        stack.has_static_eval = true;
        stack.improving = ply >= 2
            && search_stack_[ply - 2].has_static_eval
            && static_eval > search_stack_[ply - 2].corrected_static_eval;
    }

    if (!is_pv_node
        && !is_excluded_node
        && !in_check
        && !is_mate_score(beta)
        && depth <= kReverseFutilityMaxDepth
        && static_eval - reverse_futility_margin(depth, stack.improving) >= beta) {
        ++diagnostics_.reverse_futility_prunes;
        return static_eval;
    }

    if (!is_pv_node
        && !is_excluded_node
        && !in_check
        && depth <= kRazoringMaxDepth
        && static_eval + razoring_margin(depth) <= alpha) {
        ++diagnostics_.razoring_prunes;
        const int razor_score = quiescence(board, ply, alpha, beta, 0);
        if (razor_score <= alpha) {
            return razor_score;
        }
    }

    if (null_move_pruning_
        && can_try_null_move(board, depth, ply, alpha, beta, in_check, allow_null_move)
        && !is_excluded_node
        && static_eval >= beta) {
        ++diagnostics_.null_move_attempts;
        const UndoState undo = board.make_null_move();
        update_nnue_accumulator_after_null_move(ply, ply + 1);
        const int reduction = null_move_reduction(depth, static_eval, beta);
        const int null_depth = std::max(0, depth - 1 - reduction);
        const int score = -negamax(board, null_depth, ply + 1, -beta, -beta + 1, false, extensions_used);
        board.unmake_null_move(undo);
        if (should_stop()) {
            return evaluate_with_diagnostics(board, ply);
        }
        if (score >= beta) {
            ++diagnostics_.null_move_cutoffs;
            tt_->store(board.hash_key(), depth, score_to_tt(beta, ply), Bound::Lower, Move{}, raw_static_eval);
            return beta;
        }
    }

    if (!is_pv_node
        && !is_excluded_node
        && !in_check
        && depth >= kProbCutMinDepth
        && std::abs(beta) < kMateScore - kMateWindow) {
        ++diagnostics_.probcut_attempts;
        const int probcut_beta = beta + kProbCutMargin;
        MoveList noisy_moves = generate_pseudo_legal_noisy_moves(board);
        MovePicker probcut_picker{
            board,
            noisy_moves,
            MovePickerMode::ProbCut,
            Move{},
            tt_move,
            Move{},
            Move{},
            Move{},
            {},
            ply,
            true,
            history_,
            low_ply_history_,
            capture_history_,
            *continuation_history_,
            diagnostics_,
        };
        PickedMove probcut_picked;
        while (probcut_picker.next(probcut_picked)) {
            const Move& move = probcut_picked.move;
            if (!move.is_capture() && !move.is_promotion()) {
                continue;
            }
            if ((probcut_picked.see_known ? probcut_picked.see_score : static_exchange_with_diagnostics(board, move)) < 0) {
                continue;
            }
            const Color moving_side = board.side_to_move();
            const UndoState undo = board.make_move(move);
            if (!move_is_legal_after_make(board, moving_side)) {
                ++diagnostics_.illegal_pseudo_moves;
                board.unmake_move(undo);
                continue;
            }
            update_nnue_accumulator_after_move(board, undo, ply, ply + 1);
            const int reduced_depth = std::max(0, depth - 4);
            const int score = -negamax(board, reduced_depth, ply + 1, -probcut_beta, -probcut_beta + 1, false, extensions_used);
            board.unmake_move(undo);
            if (score >= probcut_beta) {
                ++diagnostics_.probcut_cutoffs;
                tt_->store(board.hash_key(), depth - 3, score_to_tt(score, ply), Bound::Lower, move, raw_static_eval);
                return score;
            }
        }
    }

    MoveList moves = in_check ? generate_pseudo_legal_check_evasions(board) : generate_pseudo_legal_moves(board);

    Move best_move;
    int best_score = -kInfinity;
    int move_index = 0;
    Move pv_move;
    const Move previous_move = ply > 0 ? search_stack_[ply - 1].current_move : Move{};
    const Piece previous_piece = ply > 0 ? search_stack_[ply - 1].moved_piece : Piece::None;
    const Move counter_move = previous_piece != Piece::None && is_valid_move_shape(previous_move)
        ? counter_moves_[piece_index(previous_piece)][previous_move.to]
        : Move{};
    std::array<ContinuationContext, kContinuationPlyOffsets.size()> continuation_contexts{};
    for (std::size_t offset_index = 0; offset_index < kContinuationPlyOffsets.size(); ++offset_index) {
        const int previous_ply = ply - kContinuationPlyOffsets[offset_index];
        if (previous_ply < 0) {
            continue;
        }
        const SearchStack& previous = search_stack_[previous_ply];
        if (previous.moved_piece == Piece::None || !is_valid_move_shape(previous.current_move)) {
            continue;
        }
        continuation_contexts[offset_index] = ContinuationContext{
            previous.moved_piece,
            previous.current_move.to,
            kContinuationScoreWeights[offset_index],
        };
    }
    MovePicker picker{
        board,
        moves,
        in_check ? MovePickerMode::Evasion : MovePickerMode::Main,
        pv_move,
        tt_move,
        ply < kMaxPly ? killer_moves_[ply][0] : Move{},
        ply < kMaxPly ? killer_moves_[ply][1] : Move{},
        counter_move,
        continuation_contexts,
        ply,
        true,
        history_,
        low_ply_history_,
        capture_history_,
        *continuation_history_,
        diagnostics_,
    };
    std::array<Move, kMaxHistoryQuiets> quiets_tried_before_cutoff{};
    std::array<Move, kMaxHistoryCaptures> captures_tried_before_cutoff{};
    std::size_t quiet_count = 0;
    std::size_t capture_count = 0;
    PickedMove picked;
    while (picker.next(picked)) {
        const Move& move = picked.move;
        if (is_excluded_node && same_move_identity(move, excluded_move)) {
            continue;
        }
        const Color moving_side = board.side_to_move();
        const Piece moved_piece = board.piece_at(move.from);
        const Piece captured_piece = captured_piece_for(board, move);
        const bool is_quiet = is_quiet_history_move(move);
        const bool is_tt_move = is_valid_move_shape(tt_move) && same_move_identity(move, tt_move);
        const bool is_pv_move = is_valid_move_shape(pv_move) && same_move_identity(move, pv_move);
        const int preliminary_stat_score = is_quiet
            ? quiet_history_score(
                history_,
                low_ply_history_,
                *continuation_history_,
                continuation_contexts,
                moving_side,
                moved_piece,
                move,
                ply)
            : capture_history_score(capture_history_, moved_piece, captured_piece, move) / 8;

        if (!is_pv_node && !in_check && !is_tt_move && !is_pv_move && best_score > -kMateScore + kMateWindow) {
            bool gives_check_for_pruning = false;
            if (depth <= 3) {
                ++diagnostics_.move_gives_check_calls;
                gives_check_for_pruning = chess::engine::move_gives_check(board, move);
            }

            if (is_quiet && !gives_check_for_pruning) {
                const int history_adjusted_lmp =
                    quiet_lmp_limit(depth, stack.improving)
                    + (preliminary_stat_score > 8'000 ? 2 : 0)
                    - (preliminary_stat_score < -8'000 ? 1 : 0);
                if (depth <= 4 && move_index >= std::max(1, history_adjusted_lmp)) {
                    ++diagnostics_.late_move_prunes;
                    continue;
                }
                const int history_futility_adjust = std::clamp(preliminary_stat_score / 96, -150, 150);
                if (depth <= kFutilityMaxDepth
                    && static_eval + futility_margin(depth, stack.improving) + history_futility_adjust <= alpha) {
                    ++diagnostics_.futility_prunes;
                    continue;
                }
                const int quiet_see_floor = -75 * depth - std::max(0, -preliminary_stat_score / 192);
                if (depth <= kQuietSeePruneMaxDepth && static_exchange_with_diagnostics(board, move) < quiet_see_floor) {
                    ++diagnostics_.futility_prunes;
                    continue;
                }
            } else if (move.is_capture()
                && !move.is_promotion()
                && depth <= kQuietSeePruneMaxDepth
                && !gives_check_for_pruning
                && (picked.see_known ? picked.see_score : static_exchange_with_diagnostics(board, move)) < -100 * depth) {
                ++diagnostics_.futility_prunes;
                continue;
            }
        }

        int singular_extension = 0;
        if (!is_excluded_node
            && is_tt_move
            && depth >= 7
            && tt_depth >= depth - 3
            && (tt_bound == Bound::Lower || tt_bound == Bound::Exact)
            && !is_mate_score(tt_score)) {
            ++diagnostics_.singular_extension_attempts;
            const int singular_beta = tt_score - 2 * depth;
            const int singular_depth = std::max(0, (depth - 1) / 2);
            const SearchStack saved_stack = stack;
            const int singular_score = negamax(
                board,
                singular_depth,
                ply,
                singular_beta - 1,
                singular_beta,
                false,
                extensions_used,
                move
            );
            stack = saved_stack;
            if (singular_score < singular_beta) {
                singular_extension = 1;
                ++diagnostics_.singular_extensions;
            } else if (!is_pv_node && singular_beta >= beta) {
                return singular_beta;
            }
        }

        const UndoState undo = board.make_move(move);
        if (!move_is_legal_after_make(board, moving_side)) {
            ++diagnostics_.illegal_pseudo_moves;
            board.unmake_move(undo);
            continue;
        }
        update_nnue_accumulator_after_move(board, undo, ply, ply + 1);
        const bool gives_check = board.in_check(board.side_to_move());
        const int extension = singular_extension + (search_extensions_
            ? extension_for_move(board, move, in_check, gives_check, moving_side, depth, extensions_used)
            : 0);
        const int child_depth = depth - 1 + extension;
        const int child_extensions_used = extensions_used + extension;
        const int stat_score = preliminary_stat_score;
        stack.current_move = move;
        stack.moved_piece = moved_piece;
        stack.captured_piece = captured_piece;
        stack.stat_score = stat_score;
        int score = 0;
        if (move_index == 0) {
            score = -negamax(board, child_depth, ply + 1, -beta, -alpha, true, child_extensions_used);
        } else {
            const int reduction = late_move_reduction(depth, move_index, stat_score, stack.improving, is_pv_node, cut_node);
            const bool can_reduce = reduction > 0
                && !in_check
                && extension == 0
                && is_quiet
                && !gives_check
                && !is_tt_move
                && !is_pv_move;
            if (can_reduce) {
                ++diagnostics_.lmr_reductions;
                const int reduced_depth = std::max(0, child_depth - reduction);
                score = -negamax(board, reduced_depth, ply + 1, -alpha - 1, -alpha, true, child_extensions_used);
                if (score > alpha && !should_stop()) {
                    ++diagnostics_.lmr_researches;
                    score = -negamax(board, child_depth, ply + 1, -alpha - 1, -alpha, true, child_extensions_used);
                }
            } else {
                score = -negamax(board, child_depth, ply + 1, -alpha - 1, -alpha, true, child_extensions_used);
            }
            if (score > alpha && score < beta) {
                score = -negamax(board, child_depth, ply + 1, -beta, -alpha, true, child_extensions_used);
            }
        }
        board.unmake_move(undo);
        ++move_index;
        stack.current_move = Move{};
        stack.moved_piece = Piece::None;
        stack.captured_piece = Piece::None;

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            ++diagnostics_.beta_cutoffs;
            diagnostics_.beta_cutoff_move_index_sum += static_cast<std::uint64_t>(move_index);
            record_cutoff(
                board,
                move,
                depth,
                ply,
                moving_side,
                moved_piece,
                captured_piece,
                quiets_tried_before_cutoff.data(),
                quiet_count,
                captures_tried_before_cutoff.data(),
                capture_count
            );
            break;
        }
        if (should_stop()) {
            break;
        }
        if (is_quiet && quiet_count < quiets_tried_before_cutoff.size()) {
            quiets_tried_before_cutoff[quiet_count] = move;
            ++quiet_count;
        } else if (move.is_capture() && capture_count < captures_tried_before_cutoff.size()) {
            captures_tried_before_cutoff[capture_count] = move;
            ++capture_count;
        }
    }

    if (move_index == 0) {
        return in_check ? -kMateScore + ply : 0;
    }

    Bound bound = Bound::Exact;
    if (best_score <= original_alpha) {
        bound = Bound::Upper;
    } else if (best_score >= original_beta) {
        bound = Bound::Lower;
    }
    if (!in_check
        && bound == Bound::Exact
        && raw_static_eval != kNoTranspositionStaticEval
        && !is_mate_score(best_score)) {
        update_correction_history(board, board.side_to_move(), depth, raw_static_eval, best_score);
    }
    if (!is_excluded_node) {
        tt_->store(board.hash_key(), depth, score_to_tt(best_score, ply), bound, best_move, raw_static_eval);
    }
    return best_score;
}

int Searcher::quiescence(Board& board, int ply, int alpha, int beta, int qply) {
    if (ply >= kMaxPly - 1) {
        return evaluate_with_diagnostics(board, kMaxPly - 1);
    }
    if (should_stop()) {
        return evaluate_with_diagnostics(board, ply);
    }
    ++nodes_;
    ++qnodes_;

    const bool in_check = board.in_check(board.side_to_move());
    if (in_check) {
        ++diagnostics_.qsearch_in_check_nodes;
    }
    int stand_pat = -kInfinity;
    if (!in_check) {
        ++diagnostics_.qsearch_stand_pat_nodes;
        stand_pat = evaluate_with_diagnostics(board, ply);
        if (stand_pat >= beta) {
            return beta;
        }
        alpha = std::max(alpha, stand_pat);
    }

    Move tt_move;
    if (const std::optional<TranspositionEntry> entry = tt_->probe_entry(board.hash_key())) {
        tt_move = entry->best_move;
    }

    const bool allow_quiet_checks = !in_check
        && qply < kMaxQuiescenceQuietCheckPly
        && ply < kMaxPly - 1
        && alpha > -kMateScore + kMateWindow
        && beta < kMateScore - kMateWindow;
    MoveList moves = in_check ? generate_pseudo_legal_check_evasions(board) : generate_pseudo_legal_noisy_moves(board);
    if (allow_quiet_checks) {
        append_moves(moves, generate_pseudo_legal_quiet_checks(board));
    }
    std::array<ContinuationContext, kContinuationPlyOffsets.size()> continuation_contexts{};
    for (std::size_t offset_index = 0; offset_index < kContinuationPlyOffsets.size(); ++offset_index) {
        const int previous_ply = ply - kContinuationPlyOffsets[offset_index];
        if (previous_ply < 0) {
            continue;
        }
        const SearchStack& previous = search_stack_[previous_ply];
        if (previous.moved_piece == Piece::None || !is_valid_move_shape(previous.current_move)) {
            continue;
        }
        continuation_contexts[offset_index] = ContinuationContext{
            previous.moved_piece,
            previous.current_move.to,
            kContinuationScoreWeights[offset_index],
        };
    }
    MovePicker picker{
        board,
        moves,
        in_check ? MovePickerMode::Evasion : MovePickerMode::QSearch,
        Move{},
        tt_move,
        ply < kMaxPly ? killer_moves_[ply][0] : Move{},
        ply < kMaxPly ? killer_moves_[ply][1] : Move{},
        Move{},
        continuation_contexts,
        ply,
        false,
        history_,
        low_ply_history_,
        capture_history_,
        *continuation_history_,
        diagnostics_,
    };

    int legal_moves_searched = 0;
    PickedMove picked;
    while (picker.next(picked)) {
        const Move& move = picked.move;
        bool gives_check = false;
        if (!in_check) {
            const bool quiet_check = !move.is_capture() && !move.is_promotion();
            if (quiet_check && !allow_quiet_checks) {
                continue;
            }
            gives_check = quiet_check;
            const bool capture_needs_check_status = move.is_capture()
                && !move.is_promotion()
                && (stand_pat + immediate_capture_gain(board, move) + kDeltaPruningMargin <= alpha
                    || needs_see_for_loss_detection(board, move));
            if (capture_needs_check_status) {
                ++diagnostics_.move_gives_check_calls;
                gives_check = chess::engine::move_gives_check(board, move);
            }
            if (move.is_capture() && !move.is_promotion()) {
                if (!gives_check && stand_pat + immediate_capture_gain(board, move) + kDeltaPruningMargin <= alpha) {
                    continue;
                }
                if (!gives_check
                    && needs_see_for_loss_detection(board, move)
                    && (picked.see_known ? picked.see_score : static_exchange_with_diagnostics(board, move)) < 0) {
                    continue;
                }
            }
        }

        const Color moving_side = board.side_to_move();
        const Piece moved_piece = board.piece_at(move.from);
        const Piece captured_piece = captured_piece_for(board, move);
        const UndoState undo = board.make_move(move);
        if (!move_is_legal_after_make(board, moving_side)) {
            ++diagnostics_.illegal_pseudo_moves;
            board.unmake_move(undo);
            continue;
        }
        update_nnue_accumulator_after_move(board, undo, ply, ply + 1);
        search_stack_[ply].current_move = move;
        search_stack_[ply].moved_piece = moved_piece;
        search_stack_[ply].captured_piece = captured_piece;
        ++legal_moves_searched;
        const int score = -quiescence(board, ply + 1, -beta, -alpha, qply + 1);
        board.unmake_move(undo);
        search_stack_[ply].current_move = Move{};
        search_stack_[ply].moved_piece = Piece::None;
        search_stack_[ply].captured_piece = Piece::None;
        if (score >= beta) {
            return beta;
        }
        alpha = std::max(alpha, score);
    }
    if (in_check && legal_moves_searched == 0) {
        return -kMateScore + ply;
    }
    return alpha;
}

int Searcher::evaluate_with_diagnostics(const Board& board, int ply) {
    ++diagnostics_.evaluations;
    const auto evaluation_start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
    const auto finish = [&](int score) {
        if (profiling_) {
            diagnostics_.evaluation_ns += elapsed_ns_since(evaluation_start);
        }
        return score;
    };
    if (eval_type_ == EvalType::Nnue && nnue_->supports_quantized_accumulator_stack() && ply >= 0 && ply < kMaxPly) {
        const auto nnue_start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
        nnue::ProfileCounters nnue_profile;
        const int white_perspective = nnue_->evaluate_white_perspective(
            board,
            nnue_accumulator_stack_[ply],
            profiling_ ? &nnue_profile : nullptr
        );
        if (profiling_) {
            ++diagnostics_.nnue_stack_evaluations;
            diagnostics_.nnue_stack_evaluation_ns += elapsed_ns_since(nnue_start);
            merge_nnue_profile(diagnostics_, nnue_profile);
        }
        return finish(board.side_to_move() == Color::White ? white_perspective : -white_perspective);
    }
    if (eval_type_ == EvalType::Hybrid && nnue_->supports_quantized_accumulator_stack() && ply >= 0 && ply < kMaxPly) {
        const auto classical_start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
        const int classical = chess::engine::evaluate_white_perspective(board);
        if (profiling_) {
            ++diagnostics_.classical_evaluations;
            diagnostics_.classical_evaluation_ns += elapsed_ns_since(classical_start);
        }
        const auto nnue_start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
        nnue::ProfileCounters nnue_profile;
        const int nnue = nnue_->evaluate_white_perspective(
            board,
            nnue_accumulator_stack_[ply],
            profiling_ ? &nnue_profile : nullptr
        );
        if (profiling_) {
            ++diagnostics_.nnue_stack_evaluations;
            diagnostics_.nnue_stack_evaluation_ns += elapsed_ns_since(nnue_start);
            merge_nnue_profile(diagnostics_, nnue_profile);
        }
        const int blended = (classical + nnue) / 2;
        return finish(board.side_to_move() == Color::White ? blended : -blended);
    }
    const auto fallback_start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
    const int score = evaluate_side_to_move(board);
    if (profiling_ && eval_type_ == EvalType::Classical) {
        ++diagnostics_.classical_evaluations;
        diagnostics_.classical_evaluation_ns += elapsed_ns_since(fallback_start);
    }
    return finish(score);
}

void Searcher::refresh_nnue_accumulator(Board& board, int ply) {
    if (ply < 0 || ply >= kMaxPly) {
        return;
    }
    nnue_accumulator_stack_[ply].valid = false;
    if (nnue_->supports_quantized_accumulator_stack()) {
        const auto start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
        nnue::ProfileCounters nnue_profile;
        nnue_->refresh_quantized_accumulator_pair(
            board,
            nnue_accumulator_stack_[ply],
            profiling_ ? &nnue_profile : nullptr
        );
        if (profiling_) {
            ++diagnostics_.nnue_accumulator_refreshes;
            diagnostics_.nnue_accumulator_refresh_ns += elapsed_ns_since(start);
            merge_nnue_profile(diagnostics_, nnue_profile);
        }
    }
}

void Searcher::update_nnue_accumulator_after_move(
    const Board& board,
    const UndoState& undo,
    int parent_ply,
    int child_ply
) {
    if (!nnue_->supports_quantized_accumulator_stack() || parent_ply < 0 || parent_ply >= kMaxPly || child_ply < 0 || child_ply >= kMaxPly) {
        return;
    }
    const auto start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
    nnue::ProfileCounters update_profile;
    const bool updated = nnue_->update_quantized_accumulator_pair_after_move(
            board,
            undo,
            nnue_accumulator_stack_[parent_ply],
            nnue_accumulator_stack_[child_ply],
            profiling_ ? &update_profile : nullptr
        );
    if (profiling_) {
        ++diagnostics_.nnue_accumulator_update_attempts;
        diagnostics_.nnue_accumulator_update_ns += elapsed_ns_since(start);
        merge_nnue_profile(diagnostics_, update_profile);
    }
    if (!updated) {
        if (profiling_) {
            ++diagnostics_.nnue_accumulator_update_fallbacks;
        }
        const auto refresh_start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
        nnue::ProfileCounters refresh_profile;
        nnue_->refresh_quantized_accumulator_pair(
            board,
            nnue_accumulator_stack_[child_ply],
            profiling_ ? &refresh_profile : nullptr
        );
        if (profiling_) {
            ++diagnostics_.nnue_accumulator_refreshes;
            diagnostics_.nnue_accumulator_refresh_ns += elapsed_ns_since(refresh_start);
            merge_nnue_profile(diagnostics_, refresh_profile);
        }
    } else if (profiling_) {
        ++diagnostics_.nnue_accumulator_update_successes;
    }
}

void Searcher::update_nnue_accumulator_after_null_move(int parent_ply, int child_ply) {
    if (!nnue_->supports_quantized_accumulator_stack() || parent_ply < 0 || parent_ply >= kMaxPly || child_ply < 0 || child_ply >= kMaxPly) {
        return;
    }
    nnue_accumulator_stack_[child_ply] = nnue_accumulator_stack_[parent_ply];
    if (profiling_) {
        ++diagnostics_.nnue_accumulator_null_copies;
    }
}

int Searcher::static_exchange_with_diagnostics(const Board& board, const Move& move) {
    ++diagnostics_.see_calls;
    return static_exchange_eval(board, move);
}

void Searcher::age_history() {
    for (auto& color_history : history_) {
        for (auto& from_history : color_history) {
            for (int& score : from_history) {
                score /= 2;
            }
        }
    }
    for (auto& ply_history : low_ply_history_) {
        for (auto& from_history : ply_history) {
            for (int& score : from_history) {
                score /= 2;
            }
        }
    }
    for (auto& piece_history : capture_history_) {
        for (auto& to_history : piece_history) {
            for (int& score : to_history) {
                score /= 2;
            }
        }
    }
    for (auto& distance_history : *continuation_history_) {
        for (auto& previous_piece_history : distance_history) {
            for (auto& previous_to_history : previous_piece_history) {
                for (auto& moved_piece_history : previous_to_history) {
                    for (int& score : moved_piece_history) {
                        score /= 2;
                    }
                }
            }
        }
    }
    const auto age_correction = [](auto& correction_history) {
        for (auto& color_correction : correction_history) {
            for (int& value : color_correction) {
                value = value * 15 / 16;
            }
        }
    };
    age_correction(pawn_correction_history_);
    age_correction(minor_correction_history_);
    age_correction(nonpawn_white_correction_history_);
    age_correction(nonpawn_black_correction_history_);
}

void Searcher::clear_history_touch_lists() {
    touched_history_.clear();
    touched_low_ply_history_.clear();
    touched_capture_history_.clear();
    touched_continuation_history_.clear();
    touched_correction_history_.clear();
}

int Searcher::corrected_static_eval(const Board& board, int raw_eval) const {
    const int side = color_index(board.side_to_move());
    int correction = 0;
    correction += pawn_correction_history_[side][pawn_correction_index(board)];
    correction += minor_correction_history_[side][minor_correction_index(board)];
    correction += nonpawn_white_correction_history_[side][nonpawn_correction_index(board, Color::White)];
    correction += nonpawn_black_correction_history_[side][nonpawn_correction_index(board, Color::Black)];
    return std::clamp(raw_eval + correction / 512, -kMateScore + kMateWindow, kMateScore - kMateWindow);
}

void Searcher::update_correction_history(const Board& board, Color side_to_move, int depth, int static_eval, int score) {
    const int delta = std::clamp(score - static_eval, -400, 400);
    if (delta == 0) {
        return;
    }
    const int bonus = correction_bonus(depth, delta);
    const int side = color_index(side_to_move);
    const int pawn_index = pawn_correction_index(board);
    const int minor_index = minor_correction_index(board);
    const int nonpawn_white_index = nonpawn_correction_index(board, Color::White);
    const int nonpawn_black_index = nonpawn_correction_index(board, Color::Black);
    update_history_score(pawn_correction_history_[side][pawn_index], bonus);
    update_history_score(minor_correction_history_[side][minor_index], bonus * 5 / 4);
    update_history_score(nonpawn_white_correction_history_[side][nonpawn_white_index], bonus * 3 / 4);
    update_history_score(nonpawn_black_correction_history_[side][nonpawn_black_index], bonus * 3 / 4);
    touched_correction_history_.push_back(encode_correction_history_key(0, side, pawn_index));
    touched_correction_history_.push_back(encode_correction_history_key(1, side, minor_index));
    touched_correction_history_.push_back(encode_correction_history_key(2, side, nonpawn_white_index));
    touched_correction_history_.push_back(encode_correction_history_key(3, side, nonpawn_black_index));
    ++diagnostics_.correction_history_updates;
}

void Searcher::merge_history_from(const Searcher& helper, int divisor) {
    divisor = std::max(2, divisor);
    for_each_unique_key(helper.touched_history_, [&](std::uint32_t key) {
        const std::size_t to = key % kBoardSquareCount;
        key /= kBoardSquareCount;
        const std::size_t from = key % kBoardSquareCount;
        key /= kBoardSquareCount;
        const std::size_t color = key;
        blend_history_value(history_[color][from][to], helper.history_[color][from][to], divisor);
    });
    for_each_unique_key(helper.touched_low_ply_history_, [&](std::uint32_t key) {
        const std::size_t to = key % kBoardSquareCount;
        key /= kBoardSquareCount;
        const std::size_t from = key % kBoardSquareCount;
        key /= kBoardSquareCount;
        const std::size_t ply = key;
        blend_history_value(low_ply_history_[ply][from][to], helper.low_ply_history_[ply][from][to], divisor);
    });
    for_each_unique_key(helper.touched_capture_history_, [&](std::uint32_t key) {
        const std::size_t captured = key % 7;
        key /= 7;
        const std::size_t to = key % kBoardSquareCount;
        key /= kBoardSquareCount;
        const std::size_t piece = key;
        blend_history_value(capture_history_[piece][to][captured], helper.capture_history_[piece][to][captured], divisor);
    });
    for (std::size_t piece = 0; piece < counter_moves_.size(); ++piece) {
        for (std::size_t to = 0; to < counter_moves_[piece].size(); ++to) {
            if (is_valid_move_shape(helper.counter_moves_[piece][to])) {
                counter_moves_[piece][to] = helper.counter_moves_[piece][to];
            }
        }
    }
    for_each_unique_key(helper.touched_continuation_history_, [&](std::uint32_t key) {
        const std::size_t to = key % kBoardSquareCount;
        key /= kBoardSquareCount;
        const std::size_t moved_piece = key % 13;
        key /= 13;
        const std::size_t previous_to = key % kBoardSquareCount;
        key /= kBoardSquareCount;
        const std::size_t previous_piece = key % 13;
        key /= 13;
        const std::size_t distance = key;
        blend_history_value(
            (*continuation_history_)[distance][previous_piece][previous_to][moved_piece][to],
            (*helper.continuation_history_)[distance][previous_piece][previous_to][moved_piece][to],
            divisor
        );
    });
    for_each_unique_key(helper.touched_correction_history_, [&](std::uint32_t key) {
        const std::size_t index = key % kCorrectionHistorySize;
        key /= kCorrectionHistorySize;
        const std::size_t color = key % 2;
        key /= 2;
        const std::size_t kind = key;
        switch (kind) {
            case 0:
                blend_history_value(pawn_correction_history_[color][index], helper.pawn_correction_history_[color][index], divisor);
                break;
            case 1:
                blend_history_value(minor_correction_history_[color][index], helper.minor_correction_history_[color][index], divisor);
                break;
            case 2:
                blend_history_value(nonpawn_white_correction_history_[color][index], helper.nonpawn_white_correction_history_[color][index], divisor);
                break;
            case 3:
                blend_history_value(nonpawn_black_correction_history_[color][index], helper.nonpawn_black_correction_history_[color][index], divisor);
                break;
            default:
                break;
        }
    });
}

void Searcher::record_cutoff(
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
) {
    if (ply >= kMaxPly) {
        return;
    }

    const int bonus = history_bonus(depth);
    const int malus = bonus;

    if (!move.is_capture() && !move.is_promotion()) {
        if (!same_move(move, killer_moves_[ply][0])) {
            killer_moves_[ply][1] = killer_moves_[ply][0];
            killer_moves_[ply][0] = move;
        }

        int& history_score = history_[color_index(side_to_move)][move.from][move.to];
        update_history_score(history_score, bonus);
        touched_history_.push_back(encode_history_key(color_index(side_to_move), move.from, move.to));
        if (ply >= 0 && ply < static_cast<int>(low_ply_history_.size())) {
            update_history_score(low_ply_history_[static_cast<std::size_t>(ply)][move.from][move.to], bonus);
            touched_low_ply_history_.push_back(encode_low_ply_history_key(ply, move.from, move.to));
        }

        for (std::size_t offset_index = 0; offset_index < kContinuationPlyOffsets.size(); ++offset_index) {
            const int previous_ply = ply - kContinuationPlyOffsets[offset_index];
            if (previous_ply < 0) {
                continue;
            }
            const SearchStack& previous = search_stack_[previous_ply];
            if (previous.moved_piece == Piece::None || !is_valid_move_shape(previous.current_move)) {
                continue;
            }
            if (offset_index == 0) {
                counter_moves_[piece_index(previous.moved_piece)][previous.current_move.to] = move;
            }
            int& continuation_score =
                (*continuation_history_)[offset_index][piece_index(previous.moved_piece)][previous.current_move.to]
                                     [piece_index(moved_piece)][move.to];
            update_history_score(continuation_score, bonus * kContinuationUpdateWeights[offset_index] / 128);
            touched_continuation_history_.push_back(encode_continuation_history_key(
                static_cast<int>(offset_index),
                previous.moved_piece,
                previous.current_move.to,
                moved_piece,
                move.to
            ));
        }

        for (std::size_t index = 0; index < quiet_count; ++index) {
            const Move& quiet = quiets_tried_before_cutoff[index];
            if (same_move_identity(quiet, move)) {
                continue;
            }
            const Piece failed_piece = board.piece_at(quiet.from);
            if (failed_piece == Piece::None) {
                continue;
            }
            int& failed_score = history_[color_index(side_to_move)][quiet.from][quiet.to];
            update_history_score(failed_score, -malus);
            touched_history_.push_back(encode_history_key(color_index(side_to_move), quiet.from, quiet.to));
            if (ply >= 0 && ply < static_cast<int>(low_ply_history_.size())) {
                update_history_score(low_ply_history_[static_cast<std::size_t>(ply)][quiet.from][quiet.to], -malus);
                touched_low_ply_history_.push_back(encode_low_ply_history_key(ply, quiet.from, quiet.to));
            }
            for (std::size_t offset_index = 0; offset_index < kContinuationPlyOffsets.size(); ++offset_index) {
                const int previous_ply = ply - kContinuationPlyOffsets[offset_index];
                if (previous_ply < 0) {
                    continue;
                }
                const SearchStack& previous = search_stack_[previous_ply];
                if (previous.moved_piece == Piece::None || !is_valid_move_shape(previous.current_move)) {
                    continue;
                }
                int& continuation_score =
                    (*continuation_history_)[offset_index][piece_index(previous.moved_piece)][previous.current_move.to]
                                         [piece_index(failed_piece)][quiet.to];
                update_history_score(continuation_score, -malus * kContinuationUpdateWeights[offset_index] / 128);
                touched_continuation_history_.push_back(encode_continuation_history_key(
                    static_cast<int>(offset_index),
                    previous.moved_piece,
                    previous.current_move.to,
                    failed_piece,
                    quiet.to
                ));
            }
        }
    } else if (move.is_capture() && captured_piece != Piece::None && moved_piece != Piece::None) {
        int& score = capture_history_[piece_index(moved_piece)][move.to][captured_type_index(captured_piece)];
        update_history_score(score, capture_history_bonus(depth));
        touched_capture_history_.push_back(encode_capture_history_key(moved_piece, move.to, captured_piece));

        const int capture_malus = capture_history_bonus(depth) / 2;
        for (std::size_t index = 0; index < capture_count; ++index) {
            const Move& capture = captures_tried_before_cutoff[index];
            if (same_move_identity(capture, move)) {
                continue;
            }
            const Piece failed_piece = board.piece_at(capture.from);
            const Piece failed_captured_piece = captured_piece_for(board, capture);
            if (failed_piece == Piece::None || failed_captured_piece == Piece::None) {
                continue;
            }
            int& failed_score = capture_history_[piece_index(failed_piece)][capture.to][captured_type_index(failed_captured_piece)];
            update_history_score(failed_score, -capture_malus);
            touched_capture_history_.push_back(encode_capture_history_key(failed_piece, capture.to, failed_captured_piece));
        }
    }
}

std::vector<Move> Searcher::extract_principal_variation(Board& board, int depth) const {
    std::vector<Move> pv;
    pv.reserve(static_cast<std::size_t>(depth));

    Board copy = board;
    for (int ply = 0; ply < depth; ++ply) {
        const std::optional<TranspositionEntry> entry = tt_->probe_entry(copy.hash_key());
        if (!entry.has_value() || !is_valid_move_shape(entry->best_move)) {
            break;
        }

        MoveList legal_moves = generate_legal_moves(copy);
        const auto found = std::find_if(legal_moves.begin(), legal_moves.end(), [&](const Move& move) {
            return move.from == entry->best_move.from
                && move.to == entry->best_move.to
                && move.promotion == entry->best_move.promotion;
        });
        if (found == legal_moves.end()) {
            break;
        }

        pv.push_back(*found);
        copy.make_move(*found);
    }
    return pv;
}

bool Searcher::should_stop() const {
    return stop_signal_->load(std::memory_order_relaxed)
        || (use_deadline_ && std::chrono::steady_clock::now() >= deadline_);
}

}  // namespace chess::engine
