#include "chess/engine/search.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <vector>

#include "chess/core/attacks.h"
#include "chess/core/movegen.h"
#include "chess/engine/evaluation.h"
#include "chess/engine/static_exchange.h"

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
constexpr int kDefaultMovesToGo = 30;
constexpr int kMinAllocatedMoveTimeMs = 10;
constexpr int kMoveTimeSafetyMs = 20;
constexpr std::size_t kMaxHistoryQuiets = 256;
constexpr std::size_t kMaxHistoryCaptures = 128;
constexpr std::size_t kNoMoveIndex = static_cast<std::size_t>(-1);

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

int color_index(Color color) {
    return static_cast<int>(color);
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

int correction_index(const Board& board) {
    return static_cast<int>(board.hash_key() & (kCorrectionHistorySize - 1));
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

    int reduction = static_cast<int>(std::log(static_cast<double>(depth)) * std::log(static_cast<double>(move_index + 1)) / 1.85);
    if (!improving) {
        ++reduction;
    }
    if (cut_node) {
        ++reduction;
    }
    if (is_pv_node) {
        --reduction;
    }
    if (stat_score > 6'000) {
        --reduction;
    } else if (stat_score < -6'000) {
        ++reduction;
    }
    return std::clamp(reduction, 0, depth - 1);
}

enum class MovePickerStage {
    PreviousPv,
    Remainder,
    Done,
};

struct PickedMove {
    Move move;
    bool see_known = false;
    int see_score = 0;
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
        Move previous_pv_move,
        Move tt_move,
        Move first_killer,
        Move second_killer,
        Move counter_move,
        Square continuation_square,
        bool use_see_for_ordering,
        const HistoryTable& history,
        const CaptureHistoryTable& capture_history,
        const ContinuationHistoryTable& continuation_history,
        SearchDiagnostics& diagnostics
    )
        : board_(board),
          side_to_move_(board.side_to_move()),
          previous_pv_move_(previous_pv_move),
          tt_move_(tt_move),
          first_killer_(first_killer),
          second_killer_(second_killer),
          counter_move_(counter_move),
          continuation_square_(continuation_square),
          use_see_for_ordering_(use_see_for_ordering),
          history_(history),
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
                    if (pick_special(previous_pv_move_, picked)) {
                        ++diagnostics_.move_picker_pv_picks;
                        return true;
                    }
                    stage_ = MovePickerStage::Remainder;
                    break;
                case MovePickerStage::Remainder:
                    if (!remainder_built_) {
                        build_remainder();
                    }
                    if (pick_from_ordered(remainder_, remainder_cursor_, picked)) {
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
    Move previous_pv_move_;
    Move tt_move_;
    Move first_killer_;
    Move second_killer_;
    Move counter_move_;
    Square continuation_square_ = kNoSquare;
    bool use_see_for_ordering_ = true;
    const HistoryTable& history_;
    const CaptureHistoryTable& capture_history_;
    const ContinuationHistoryTable& continuation_history_;
    SearchDiagnostics& diagnostics_;
    FixedVector<ScoredMove, MoveList::kCapacity> moves_;
    FixedVector<ScoredIndex, MoveList::kCapacity> remainder_;
    MovePickerStage stage_ = MovePickerStage::PreviousPv;
    std::size_t remainder_cursor_ = 0;
    bool remainder_built_ = false;

    void build_remainder() {
        remainder_built_ = true;
        for (std::size_t index = 0; index < moves_.size(); ++index) {
            ScoredMove& state = moves_[index];
            if (state.emitted) {
                continue;
            }
            remainder_.emplace_back(index, move_order_score(state));
        }
        sort_descending(remainder_);
    }

    void sort_descending(FixedVector<ScoredIndex, MoveList::kCapacity>& indices) {
        std::stable_sort(indices.begin(), indices.end(), [](const ScoredIndex& lhs, const ScoredIndex& rhs) {
            return lhs.score > rhs.score;
        });
    }

    int quiet_move_order_score(ScoredMove& state) {
        int score = 0;
        if (same_move(state.move, first_killer_)) {
            score += 30'000;
        } else if (same_move(state.move, second_killer_)) {
            score += 29'000;
        } else if (same_move_identity(state.move, counter_move_)) {
            score += 28'000;
        }
        score += quiet_score(state);
        return score;
    }

    int move_order_score(ScoredMove& state) {
        if (is_valid_move_shape(tt_move_) && same_move_identity(state.move, tt_move_)) {
            return 1'000'000;
        }
        classify_tactical(state);
        return state.is_tactical ? state.tactical_score : quiet_move_order_score(state);
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
        } else if (same_move(state.move, first_killer_) || same_move(state.move, second_killer_)) {
            ++diagnostics_.move_picker_killer_picks;
        } else {
            ++diagnostics_.move_picker_quiet_picks;
        }
        if (is_valid_move_shape(tt_move_) && same_move_identity(state.move, tt_move_)) {
            ++diagnostics_.move_picker_tt_picks;
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

            if (exchange_score >= 0) {
                state.tactical_score += 70'000 + exchange_score + mvv_lva_score(board_, state.move);
            } else {
                state.tactical_score += 5'000 + exchange_score + mvv_lva_score(board_, state.move);
            }
            state.tactical_score += capture_history_[color_index(side_to_move_)][state.move.from][state.move.to] / 16;
        }
    }

    int quiet_score(ScoredMove& state) {
        if (!state.quiet_score_known) {
            state.quiet_score_known = true;
            state.quiet_score = history_[color_index(side_to_move_)][state.move.from][state.move.to];
            if (is_valid_square(continuation_square_)) {
                state.quiet_score += continuation_history_[color_index(side_to_move_)][continuation_square_][state.move.from][state.move.to] / 2;
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

std::chrono::milliseconds allocated_time(const Board& board, const SearchLimits& limits) {
    if (limits.move_time.count() > 0) {
        return limits.move_time;
    }

    const bool white_to_move = board.side_to_move() == Color::White;
    const std::chrono::milliseconds remaining = white_to_move ? limits.white_time : limits.black_time;
    const std::chrono::milliseconds increment = white_to_move ? limits.white_increment : limits.black_increment;
    if (remaining.count() <= 0) {
        return std::chrono::milliseconds{0};
    }

    const int moves_to_go = limits.moves_to_go > 0 ? limits.moves_to_go : kDefaultMovesToGo;
    const std::int64_t safety = std::min<std::int64_t>(kMoveTimeSafetyMs, std::max<std::int64_t>(0, remaining.count() / 10));
    const std::int64_t usable = std::max<std::int64_t>(1, remaining.count() - safety);
    const std::int64_t base = usable / moves_to_go;
    const std::int64_t increment_part = increment.count() * 3 / 4;
    const std::int64_t cap = std::max<std::int64_t>(1, usable / 4);
    const std::int64_t allocated = std::clamp<std::int64_t>(
        base + increment_part,
        std::min<std::int64_t>(kMinAllocatedMoveTimeMs, usable),
        cap
    );
    return std::chrono::milliseconds{allocated};
}

}  // namespace

Searcher::Searcher() {
    clear();
}

SearchResult Searcher::search(Board& board, const SearchLimits& limits) {
    nodes_ = 0;
    qnodes_ = 0;
    tt_hits_ = 0;
    diagnostics_ = {};
    const std::chrono::milliseconds effective_move_time = allocated_time(board, limits);
    use_deadline_ = effective_move_time.count() > 0;
    start_time_ = std::chrono::steady_clock::now();
    deadline_ = start_time_ + effective_move_time;
    tt_.new_search();
    age_history();
    previous_iteration_pv_.clear();
    refresh_nnue_accumulator(board, 0);

    SearchResult result;
    const MoveList legal_moves = generate_legal_moves(board);
    root_search_moves_constrained_ = !limits.search_moves.empty();
    root_search_moves_.clear();
    if (root_search_moves_constrained_) {
        for (const Move& move : legal_moves) {
            if (contains_move_identity(limits.search_moves, move)) {
                root_search_moves_.push_back(move);
            }
        }
    } else {
        root_search_moves_ = legal_moves;
    }

    if (root_search_moves_.empty()) {
        result.best_move = Move{};
        result.score_centipawns = legal_moves.empty() && board.in_check(board.side_to_move()) ? -kMateScore : 0;
        result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_
        );
        return result;
    }

    const int max_depth = std::min(std::max(1, limits.depth), kMaxPly - 1);
    for (int depth = 1; depth <= max_depth; ++depth) {
        if (should_stop()) {
            break;
        }

        previous_iteration_pv_ = result.principal_variation;

        int alpha = -kInfinity;
        int beta = kInfinity;
        int window = kAspirationWindow;
        if (depth > 1 && !is_mate_score(result.score_centipawns)) {
            alpha = std::max(-kInfinity, result.score_centipawns - window);
            beta = std::min(kInfinity, result.score_centipawns + window);
        }

        int score = 0;
        while (true) {
            score = negamax(board, depth, 0, alpha, beta, true, 0);
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
            const TranspositionEntry* root_entry = tt_.probe(board.hash_key());
            if (root_entry != nullptr && is_valid_move_shape(root_entry->best_move)) {
                result.best_move = root_entry->best_move;
            } else if (!is_valid_move_shape(result.best_move)) {
                result.best_move = root_search_moves_.front();
            }
            result.score_centipawns = score;
            result.depth = depth;
            result.nodes = nodes_;
            result.qnodes = qnodes_;
            result.tt_hits = tt_hits_;
            result.diagnostics = diagnostics_;
            result.principal_variation = extract_principal_variation(board, depth);
            if (!result.principal_variation.empty()) {
                result.best_move = result.principal_variation.front();
            }
            previous_iteration_pv_ = result.principal_variation;
        }
    }

    if (!is_valid_move_shape(result.best_move)) {
        result.best_move = root_search_moves_.front();
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
    tt_.resize_mb(megabytes);
}

std::size_t Searcher::hash_size_mb() const {
    return tt_.size_mb();
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

void Searcher::set_eval_type(EvalType type) {
    eval_type_ = type;
}

EvalType Searcher::eval_type() const {
    return eval_type_;
}

bool Searcher::load_nnue(const std::filesystem::path& path, std::string* error) {
    return nnue_.load(path, error);
}

void Searcher::clear_nnue() {
    nnue_.clear();
}

bool Searcher::nnue_loaded() const {
    return nnue_.loaded();
}

const nnue::ModelInfo& Searcher::nnue_info() const {
    return nnue_.info();
}

int Searcher::evaluate_nnue_white_perspective(const Board& board) const {
    if (!nnue_.loaded()) {
        return chess::engine::evaluate_white_perspective(board);
    }
    return nnue_.evaluate_white_perspective(board);
}

int Searcher::evaluate_white_perspective(const Board& board) const {
    return chess::engine::evaluate_white_perspective(board, EvaluationConfig{eval_type_, &nnue_, 50});
}

int Searcher::evaluate_side_to_move(const Board& board) const {
    return evaluate(board, EvaluationConfig{eval_type_, &nnue_, 50});
}

void Searcher::clear() {
    tt_.clear();
    for (auto& ply_killers : killer_moves_) {
        ply_killers = {Move{}, Move{}};
    }
    for (auto& color_history : history_) {
        for (auto& from_history : color_history) {
            from_history.fill(0);
        }
    }
    for (auto& color_history : capture_history_) {
        for (auto& from_history : color_history) {
            from_history.fill(0);
        }
    }
    for (auto& color_counters : counter_moves_) {
        for (auto& from_counters : color_counters) {
            from_counters.fill(Move{});
        }
    }
    for (auto& color_history : continuation_history_) {
        for (auto& previous_to_history : color_history) {
            for (auto& from_history : previous_to_history) {
                from_history.fill(0);
            }
        }
    }
    for (auto& color_correction : correction_history_) {
        color_correction.fill(0);
    }
    for (auto& stack : search_stack_) {
        stack = SearchStack{};
    }
    previous_iteration_pv_.clear();
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
    const bool is_root = ply == 0;
    const bool is_pv_node = beta - alpha > 1;
    const bool is_excluded_node = is_valid_move_shape(excluded_move);
    const bool cut_node = !is_pv_node;
    SearchStack& stack = search_stack_[ply];
    stack = SearchStack{};

    if (depth == 0 || ply >= kMaxPly - 1) {
        return quiescence(board, ply, alpha, beta, 0);
    }

    const bool in_check = board.in_check(board.side_to_move());
    Move tt_move;
    int tt_score = 0;
    int tt_static_eval = kNoTranspositionStaticEval;
    Bound tt_bound = Bound::Exact;
    int tt_depth = -1;
    const bool use_tt_entry = !is_excluded_node && !(is_root && root_search_moves_constrained_);
    if (use_tt_entry) {
        const TranspositionEntry* entry = tt_.probe(board.hash_key());
        if (entry != nullptr) {
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

    if (!is_root
        && !is_pv_node
        && !is_excluded_node
        && !in_check
        && !is_mate_score(beta)
        && depth <= kReverseFutilityMaxDepth
        && static_eval - reverse_futility_margin(depth, stack.improving) >= beta) {
        ++diagnostics_.reverse_futility_prunes;
        return static_eval;
    }

    if (!is_root
        && !is_pv_node
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
            tt_.store(board.hash_key(), depth, score_to_tt(beta, ply), Bound::Lower, Move{}, raw_static_eval);
            return beta;
        }
    }

    if (!is_root
        && !is_pv_node
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
            Move{},
            tt_move,
            Move{},
            Move{},
            Move{},
            kNoSquare,
            true,
            history_,
            capture_history_,
            continuation_history_,
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
                tt_.store(board.hash_key(), depth - 3, score_to_tt(score, ply), Bound::Lower, move, raw_static_eval);
                return score;
            }
        }
    }

    MoveList moves = is_root
        ? root_search_moves_
        : (in_check ? generate_pseudo_legal_check_evasions(board) : generate_pseudo_legal_moves(board));

    Move best_move;
    int best_score = -kInfinity;
    int move_index = 0;
    Move pv_move;
    if (ply == 0 && !previous_iteration_pv_.empty()) {
        pv_move = previous_iteration_pv_.front();
    }
    const Move previous_move = ply > 0 ? search_stack_[ply - 1].current_move : Move{};
    const Move counter_move = is_valid_move_shape(previous_move)
        ? counter_moves_[color_index(board.side_to_move())][previous_move.from][previous_move.to]
        : Move{};
    const Square continuation_square = is_valid_move_shape(previous_move) ? previous_move.to : kNoSquare;
    MovePicker picker{
        board,
        moves,
        pv_move,
        tt_move,
        ply < kMaxPly ? killer_moves_[ply][0] : Move{},
        ply < kMaxPly ? killer_moves_[ply][1] : Move{},
        counter_move,
        continuation_square,
        true,
        history_,
        capture_history_,
        continuation_history_,
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

        if (!is_root && !is_pv_node && !in_check && !is_tt_move && !is_pv_move && best_score > -kMateScore + kMateWindow) {
            bool gives_check_for_pruning = false;
            if (depth <= 3) {
                ++diagnostics_.move_gives_check_calls;
                gives_check_for_pruning = chess::engine::move_gives_check(board, move);
            }

            if (is_quiet && !gives_check_for_pruning) {
                if (depth <= 4 && move_index >= quiet_lmp_limit(depth, stack.improving)) {
                    ++diagnostics_.late_move_prunes;
                    continue;
                }
                if (depth <= kFutilityMaxDepth && static_eval + futility_margin(depth, stack.improving) <= alpha) {
                    ++diagnostics_.futility_prunes;
                    continue;
                }
                if (depth <= kQuietSeePruneMaxDepth && static_exchange_with_diagnostics(board, move) < -75 * depth) {
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
        if (!is_root
            && !is_excluded_node
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
        const int stat_score = is_quiet
            ? history_[color_index(moving_side)][move.from][move.to]
                + (is_valid_square(continuation_square)
                    ? continuation_history_[color_index(moving_side)][continuation_square][move.from][move.to] / 2
                    : 0)
            : capture_history_[color_index(moving_side)][move.from][move.to] / 16;
        stack.current_move = move;
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

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            ++diagnostics_.beta_cutoffs;
            diagnostics_.beta_cutoff_move_index_sum += static_cast<std::uint64_t>(move_index);
            record_cutoff(
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
        tt_.store(board.hash_key(), depth, score_to_tt(best_score, ply), bound, best_move, raw_static_eval);
    }
    return best_score;
}

int Searcher::quiescence(Board& board, int ply, int alpha, int beta, int qply) {
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
    if (const TranspositionEntry* entry = tt_.probe(board.hash_key())) {
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
    MovePicker picker{
        board,
        moves,
        Move{},
        tt_move,
        ply < kMaxPly ? killer_moves_[ply][0] : Move{},
        ply < kMaxPly ? killer_moves_[ply][1] : Move{},
        Move{},
        ply > 0 && is_valid_move_shape(search_stack_[ply - 1].current_move) ? search_stack_[ply - 1].current_move.to : kNoSquare,
        false,
        history_,
        capture_history_,
        continuation_history_,
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
        const UndoState undo = board.make_move(move);
        if (!move_is_legal_after_make(board, moving_side)) {
            ++diagnostics_.illegal_pseudo_moves;
            board.unmake_move(undo);
            continue;
        }
        update_nnue_accumulator_after_move(board, undo, ply, ply + 1);
        ++legal_moves_searched;
        const int score = -quiescence(board, ply + 1, -beta, -alpha, qply + 1);
        board.unmake_move(undo);
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
    if (eval_type_ == EvalType::Nnue && nnue_.supports_quantized_accumulator_stack() && ply >= 0 && ply < kMaxPly) {
        const auto nnue_start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
        nnue::ProfileCounters nnue_profile;
        const int white_perspective = nnue_.evaluate_white_perspective(
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
    if (eval_type_ == EvalType::Hybrid && nnue_.supports_quantized_accumulator_stack() && ply >= 0 && ply < kMaxPly) {
        const auto classical_start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
        const int classical = chess::engine::evaluate_white_perspective(board);
        if (profiling_) {
            ++diagnostics_.classical_evaluations;
            diagnostics_.classical_evaluation_ns += elapsed_ns_since(classical_start);
        }
        const auto nnue_start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
        nnue::ProfileCounters nnue_profile;
        const int nnue = nnue_.evaluate_white_perspective(
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
    if (nnue_.supports_quantized_accumulator_stack()) {
        const auto start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
        nnue::ProfileCounters nnue_profile;
        nnue_.refresh_quantized_accumulator_pair(
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
    if (!nnue_.supports_quantized_accumulator_stack() || parent_ply < 0 || parent_ply >= kMaxPly || child_ply < 0 || child_ply >= kMaxPly) {
        return;
    }
    const auto start = profiling_ ? ProfileClock::now() : ProfileClock::time_point{};
    nnue::ProfileCounters update_profile;
    const bool updated = nnue_.update_quantized_accumulator_pair_after_move(
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
        nnue_.refresh_quantized_accumulator_pair(
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
    if (!nnue_.supports_quantized_accumulator_stack() || parent_ply < 0 || parent_ply >= kMaxPly || child_ply < 0 || child_ply >= kMaxPly) {
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
    for (auto& color_history : capture_history_) {
        for (auto& from_history : color_history) {
            for (int& score : from_history) {
                score /= 2;
            }
        }
    }
    for (auto& color_history : continuation_history_) {
        for (auto& previous_to_history : color_history) {
            for (auto& from_history : previous_to_history) {
                for (int& score : from_history) {
                    score /= 2;
                }
            }
        }
    }
    for (auto& color_correction : correction_history_) {
        for (int& value : color_correction) {
            value = value * 15 / 16;
        }
    }
}

int Searcher::corrected_static_eval(const Board& board, int raw_eval) const {
    const int correction = correction_history_[color_index(board.side_to_move())][correction_index(board)];
    return std::clamp(raw_eval + correction / 128, -kMateScore + kMateWindow, kMateScore - kMateWindow);
}

void Searcher::update_correction_history(const Board& board, Color side_to_move, int depth, int static_eval, int score) {
    const int delta = std::clamp(score - static_eval, -400, 400);
    if (delta == 0) {
        return;
    }
    int& entry = correction_history_[color_index(side_to_move)][correction_index(board)];
    update_history_score(entry, correction_bonus(depth, delta));
    ++diagnostics_.correction_history_updates;
}

void Searcher::record_cutoff(
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

        if (ply > 0 && is_valid_move_shape(search_stack_[ply - 1].current_move)) {
            const Move previous_move = search_stack_[ply - 1].current_move;
            counter_moves_[color_index(side_to_move)][previous_move.from][previous_move.to] = move;
            int& continuation_score =
                continuation_history_[color_index(side_to_move)][previous_move.to][move.from][move.to];
            update_history_score(continuation_score, bonus);
        }

        for (std::size_t index = 0; index < quiet_count; ++index) {
            const Move& quiet = quiets_tried_before_cutoff[index];
            if (same_move_identity(quiet, move)) {
                continue;
            }
            int& failed_score = history_[color_index(side_to_move)][quiet.from][quiet.to];
            update_history_score(failed_score, -malus);
            if (ply > 0 && is_valid_move_shape(search_stack_[ply - 1].current_move)) {
                const Move previous_move = search_stack_[ply - 1].current_move;
                int& continuation_score =
                    continuation_history_[color_index(side_to_move)][previous_move.to][quiet.from][quiet.to];
                update_history_score(continuation_score, -malus);
            }
        }
    } else if (move.is_capture() && captured_piece != Piece::None && moved_piece != Piece::None) {
        int& score = capture_history_[color_index(side_to_move)][move.from][move.to];
        update_history_score(score, capture_history_bonus(depth));

        const int capture_malus = capture_history_bonus(depth) / 2;
        for (std::size_t index = 0; index < capture_count; ++index) {
            const Move& capture = captures_tried_before_cutoff[index];
            if (same_move_identity(capture, move)) {
                continue;
            }
            int& failed_score = capture_history_[color_index(side_to_move)][capture.from][capture.to];
            update_history_score(failed_score, -capture_malus);
        }
    }
}

std::vector<Move> Searcher::extract_principal_variation(Board& board, int depth) const {
    std::vector<Move> pv;
    pv.reserve(static_cast<std::size_t>(depth));

    Board copy = board;
    for (int ply = 0; ply < depth; ++ply) {
        const TranspositionEntry* entry = tt_.probe(copy.hash_key());
        if (entry == nullptr || !is_valid_move_shape(entry->best_move)) {
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
    return use_deadline_ && std::chrono::steady_clock::now() >= deadline_;
}

}  // namespace chess::engine
