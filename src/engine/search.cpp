#include "chess/engine/search.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
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
constexpr int kNullMoveReduction = 3;
constexpr int kNullMoveMinMaterial = 500;
constexpr int kMaxExtensionsPerLine = 6;
constexpr int kMaxQuiescenceQuietCheckPly = 1;
constexpr int kDeltaPruningMargin = 200;
constexpr int kDefaultMovesToGo = 30;
constexpr int kMinAllocatedMoveTimeMs = 10;
constexpr int kMoveTimeSafetyMs = 20;
constexpr std::size_t kMaxHistoryQuiets = 256;
constexpr std::size_t kNoMoveIndex = static_cast<std::size_t>(-1);

using HistoryTable = std::array<std::array<std::array<int, kBoardSquareCount>, kBoardSquareCount>, 2>;

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
        bool use_see_for_ordering,
        const HistoryTable& history,
        SearchDiagnostics& diagnostics
    )
        : board_(board),
          side_to_move_(board.side_to_move()),
          previous_pv_move_(previous_pv_move),
          tt_move_(tt_move),
          first_killer_(first_killer),
          second_killer_(second_killer),
          use_see_for_ordering_(use_see_for_ordering),
          history_(history),
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
    bool use_see_for_ordering_ = true;
    const HistoryTable& history_;
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
        }
    }

    int quiet_score(ScoredMove& state) {
        if (!state.quiet_score_known) {
            state.quiet_score_known = true;
            state.quiet_score = history_[color_index(side_to_move_)][state.move.from][state.move.to];
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
    previous_iteration_pv_.clear();
}

int Searcher::negamax(Board& board, int depth, int ply, int alpha, int beta, bool allow_null_move, int extensions_used) {
    if (should_stop()) {
        return evaluate_with_diagnostics(board, ply);
    }
    ++nodes_;

    const int original_alpha = alpha;
    const int original_beta = beta;
    Move tt_move;
    const bool use_tt_entry = !(ply == 0 && root_search_moves_constrained_);
    if (use_tt_entry) {
        const TranspositionEntry* entry = tt_.probe(board.hash_key());
        if (entry != nullptr) {
            tt_move = entry->best_move;
            if (entry->depth >= depth) {
                ++tt_hits_;
                const int tt_score = score_from_tt(entry->score, ply);
                if (entry->bound == Bound::Exact) {
                    return tt_score;
                }
                if (entry->bound == Bound::Lower) {
                    alpha = std::max(alpha, tt_score);
                } else if (entry->bound == Bound::Upper) {
                    beta = std::min(beta, tt_score);
                }
                if (alpha >= beta) {
                    return tt_score;
                }
            }
        }
    }

    if (depth == 0 || ply >= kMaxPly - 1) {
        return quiescence(board, ply, alpha, beta, 0);
    }

    const bool in_check = board.in_check(board.side_to_move());
    MoveList moves = ply == 0
        ? root_search_moves_
        : (in_check ? generate_pseudo_legal_check_evasions(board) : generate_pseudo_legal_moves(board));

    if (null_move_pruning_
        && can_try_null_move(board, depth, ply, alpha, beta, in_check, allow_null_move)
        && evaluate_with_diagnostics(board, ply) >= beta) {
        ++diagnostics_.null_move_attempts;
        const UndoState undo = board.make_null_move();
        update_nnue_accumulator_after_null_move(ply, ply + 1);
        const int null_depth = std::max(0, depth - 1 - kNullMoveReduction);
        const int score = -negamax(board, null_depth, ply + 1, -beta, -beta + 1, false, extensions_used);
        board.unmake_null_move(undo);
        if (should_stop()) {
            return evaluate_with_diagnostics(board, ply);
        }
        if (score >= beta) {
            ++diagnostics_.null_move_cutoffs;
            tt_.store(board.hash_key(), depth, score_to_tt(beta, ply), Bound::Lower, Move{});
            return beta;
        }
    }

    Move best_move;
    int best_score = -kInfinity;
    int move_index = 0;
    Move pv_move;
    if (ply == 0 && !previous_iteration_pv_.empty()) {
        pv_move = previous_iteration_pv_.front();
    }
    MovePicker picker{
        board,
        moves,
        pv_move,
        tt_move,
        ply < kMaxPly ? killer_moves_[ply][0] : Move{},
        ply < kMaxPly ? killer_moves_[ply][1] : Move{},
        true,
        history_,
        diagnostics_,
    };
    std::array<Move, kMaxHistoryQuiets> quiets_tried_before_cutoff{};
    std::size_t quiet_count = 0;
    PickedMove picked;
    while (picker.next(picked)) {
        const Move& move = picked.move;
        const Color moving_side = board.side_to_move();
        const UndoState undo = board.make_move(move);
        if (!move_is_legal_after_make(board, moving_side)) {
            ++diagnostics_.illegal_pseudo_moves;
            board.unmake_move(undo);
            continue;
        }
        update_nnue_accumulator_after_move(board, undo, ply, ply + 1);
        const bool gives_check = board.in_check(board.side_to_move());
        const int extension = search_extensions_
            ? extension_for_move(board, move, in_check, gives_check, moving_side, depth, extensions_used)
            : 0;
        const int child_depth = depth - 1 + extension;
        const int child_extensions_used = extensions_used + extension;
        int score = 0;
        if (move_index == 0) {
            score = -negamax(board, child_depth, ply + 1, -beta, -alpha, true, child_extensions_used);
        } else {
            const bool can_reduce = depth >= kLmrMinDepth
                && move_index >= kLmrMoveIndex
                && !in_check
                && extension == 0
                && is_quiet_history_move(move)
                && !gives_check
                && !(is_valid_move_shape(tt_move) && same_move_identity(move, tt_move))
                && !(is_valid_move_shape(pv_move) && same_move_identity(move, pv_move));
            if (can_reduce) {
                ++diagnostics_.lmr_reductions;
                score = -negamax(board, depth - 2, ply + 1, -alpha - 1, -alpha, true, child_extensions_used);
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

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            ++diagnostics_.beta_cutoffs;
            diagnostics_.beta_cutoff_move_index_sum += static_cast<std::uint64_t>(move_index);
            record_cutoff(move, depth, ply, moving_side, quiets_tried_before_cutoff.data(), quiet_count);
            break;
        }
        if (should_stop()) {
            break;
        }
        if (is_quiet_history_move(move) && quiet_count < quiets_tried_before_cutoff.size()) {
            quiets_tried_before_cutoff[quiet_count] = move;
            ++quiet_count;
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
    tt_.store(board.hash_key(), depth, score_to_tt(best_score, ply), bound, best_move);
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
        false,
        history_,
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
    if (eval_type_ == EvalType::Nnue && nnue_.supports_quantized_accumulator_stack() && ply >= 0 && ply < kMaxPly) {
        return board.side_to_move() == Color::White
            ? nnue_.evaluate_white_perspective(board, nnue_accumulator_stack_[ply])
            : -nnue_.evaluate_white_perspective(board, nnue_accumulator_stack_[ply]);
    }
    if (eval_type_ == EvalType::Hybrid && nnue_.supports_quantized_accumulator_stack() && ply >= 0 && ply < kMaxPly) {
        const int classical = chess::engine::evaluate_white_perspective(board);
        const int nnue = nnue_.evaluate_white_perspective(board, nnue_accumulator_stack_[ply]);
        const int blended = (classical + nnue) / 2;
        return board.side_to_move() == Color::White ? blended : -blended;
    }
    return evaluate_side_to_move(board);
}

void Searcher::refresh_nnue_accumulator(Board& board, int ply) {
    if (ply < 0 || ply >= kMaxPly) {
        return;
    }
    nnue_accumulator_stack_[ply].valid = false;
    if (nnue_.supports_quantized_accumulator_stack()) {
        nnue_.refresh_quantized_accumulator_pair(board, nnue_accumulator_stack_[ply]);
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
    if (!nnue_.update_quantized_accumulator_pair_after_move(
            board,
            undo,
            nnue_accumulator_stack_[parent_ply],
            nnue_accumulator_stack_[child_ply]
        )) {
        nnue_.refresh_quantized_accumulator_pair(board, nnue_accumulator_stack_[child_ply]);
    }
}

void Searcher::update_nnue_accumulator_after_null_move(int parent_ply, int child_ply) {
    if (!nnue_.supports_quantized_accumulator_stack() || parent_ply < 0 || parent_ply >= kMaxPly || child_ply < 0 || child_ply >= kMaxPly) {
        return;
    }
    nnue_accumulator_stack_[child_ply] = nnue_accumulator_stack_[parent_ply];
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
}

void Searcher::record_cutoff(
    const Move& move,
    int depth,
    int ply,
    Color side_to_move,
    const Move* quiets_tried_before_cutoff,
    std::size_t quiet_count
) {
    if (move.is_capture() || move.is_promotion() || ply >= kMaxPly) {
        return;
    }

    if (!same_move(move, killer_moves_[ply][0])) {
        killer_moves_[ply][1] = killer_moves_[ply][0];
        killer_moves_[ply][0] = move;
    }

    const int bonus = depth * depth;
    const int malus = bonus;
    int& history_score = history_[color_index(side_to_move)][move.from][move.to];
    history_score = clamp_history(history_score + bonus);

    for (std::size_t index = 0; index < quiet_count; ++index) {
        const Move& quiet = quiets_tried_before_cutoff[index];
        if (same_move_identity(quiet, move)) {
            continue;
        }
        int& failed_score = history_[color_index(side_to_move)][quiet.from][quiet.to];
        failed_score = clamp_history(failed_score - malus);
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
