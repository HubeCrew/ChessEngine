#include "chess/engine/search.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "chess/core/movegen.h"
#include "chess/engine/evaluation.h"
#include "chess/engine/static_exchange.h"

namespace chess::engine {

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

bool move_gives_check(Board& board, const Move& move) {
    const Color moving_side = board.side_to_move();
    const UndoState undo = board.make_move(move);
    const bool gives_check = board.in_check(opposite(moving_side));
    board.unmake_move(undo);
    return gives_check;
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

bool is_quiescence_candidate(Board& board, const Move& move, bool allow_quiet_checks) {
    return move.is_capture() || move.is_promotion() || (allow_quiet_checks && move_gives_check(board, move));
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
    const std::chrono::milliseconds effective_move_time = allocated_time(board, limits);
    use_deadline_ = effective_move_time.count() > 0;
    start_time_ = std::chrono::steady_clock::now();
    deadline_ = start_time_ + effective_move_time;
    tt_.new_search();
    age_history();
    previous_iteration_pv_.clear();

    SearchResult result;
    MoveList moves = generate_legal_moves(board);
    if (moves.empty()) {
        result.best_move = Move{};
        result.score_centipawns = board.in_check(board.side_to_move()) ? -kMateScore : 0;
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
                result.best_move = moves.front();
            }
            result.score_centipawns = score;
            result.depth = depth;
            result.nodes = nodes_;
            result.qnodes = qnodes_;
            result.tt_hits = tt_hits_;
            result.principal_variation = extract_principal_variation(board, depth);
            if (!result.principal_variation.empty()) {
                result.best_move = result.principal_variation.front();
            }
            previous_iteration_pv_ = result.principal_variation;
        }
    }

    if (!is_valid_move_shape(result.best_move)) {
        result.best_move = moves.front();
    }

    result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time_
    );
    const auto elapsed_ms = std::max<std::int64_t>(1, result.elapsed.count());
    result.nps = result.nodes * 1000ULL / static_cast<std::uint64_t>(elapsed_ms);
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
        return evaluate(board);
    }
    ++nodes_;

    const int original_alpha = alpha;
    const int original_beta = beta;
    Move tt_move;
    if (const TranspositionEntry* entry = tt_.probe(board.hash_key())) {
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

    if (depth == 0 || ply >= kMaxPly - 1) {
        return quiescence(board, ply, alpha, beta, 0);
    }

    const bool in_check = board.in_check(board.side_to_move());
    MoveList moves = generate_legal_moves(board);
    if (moves.empty()) {
        return in_check ? -kMateScore + ply : 0;
    }

    if (null_move_pruning_
        && can_try_null_move(board, depth, ply, alpha, beta, in_check, allow_null_move)
        && evaluate(board) >= beta) {
        const UndoState undo = board.make_null_move();
        const int null_depth = std::max(0, depth - 1 - kNullMoveReduction);
        const int score = -negamax(board, null_depth, ply + 1, -beta, -beta + 1, false, extensions_used);
        board.unmake_null_move(undo);
        if (should_stop()) {
            return evaluate(board);
        }
        if (score >= beta) {
            tt_.store(board.hash_key(), depth, score_to_tt(beta, ply), Bound::Lower, Move{});
            return beta;
        }
    }

    order_moves(board, moves, tt_move, ply);

    Move best_move;
    int best_score = -kInfinity;
    int move_index = 0;
    Move pv_move;
    if (ply == 0 && !previous_iteration_pv_.empty()) {
        pv_move = previous_iteration_pv_.front();
    }
    std::array<Move, kMaxHistoryQuiets> quiets_tried_before_cutoff{};
    std::size_t quiet_count = 0;
    for (const Move& move : moves) {
        const Color moving_side = board.side_to_move();
        const UndoState undo = board.make_move(move);
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
                score = -negamax(board, depth - 2, ply + 1, -alpha - 1, -alpha, true, child_extensions_used);
                if (score > alpha && !should_stop()) {
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
        return evaluate(board);
    }
    ++nodes_;
    ++qnodes_;

    const bool in_check = board.in_check(board.side_to_move());
    int stand_pat = -kInfinity;
    if (!in_check) {
        stand_pat = evaluate(board);
        if (stand_pat >= beta) {
            return beta;
        }
        alpha = std::max(alpha, stand_pat);
    }

    Move tt_move;
    if (const TranspositionEntry* entry = tt_.probe(board.hash_key())) {
        tt_move = entry->best_move;
    }

    MoveList moves = generate_legal_moves(board);
    if (!in_check) {
        const bool allow_quiet_checks = qply < kMaxQuiescenceQuietCheckPly
            && ply < kMaxPly - 1
            && alpha > -kMateScore + kMateWindow
            && beta < kMateScore - kMateWindow;
        moves.erase(
            std::remove_if(moves.begin(), moves.end(), [&](const Move& move) {
                return !is_quiescence_candidate(board, move, allow_quiet_checks);
            }),
            moves.end()
        );
    } else if (moves.empty()) {
        return -kMateScore + ply;
    }
    order_moves(board, moves, tt_move, ply);

    for (const Move& move : moves) {
        bool gives_check = false;
        if (!in_check) {
            gives_check = move_gives_check(board, move);
            if (move.is_capture() && !move.is_promotion() && !gives_check
                && stand_pat + immediate_capture_gain(board, move) + kDeltaPruningMargin <= alpha) {
                continue;
            }
            if (needs_see_for_loss_detection(board, move)
                && static_exchange_eval(board, move) < 0 && !gives_check) {
                continue;
            }
        }

        const UndoState undo = board.make_move(move);
        const int score = -quiescence(board, ply + 1, -beta, -alpha, qply + 1);
        board.unmake_move(undo);
        if (score >= beta) {
            return beta;
        }
        alpha = std::max(alpha, score);
    }
    return alpha;
}

void Searcher::order_moves(Board& board, MoveList& moves, const Move& tt_move, int ply) const {
    const Color side = board.side_to_move();
    Move pv_move;
    if (ply == 0 && !previous_iteration_pv_.empty()) {
        pv_move = previous_iteration_pv_.front();
    }

    struct ScoredMove {
        Move move;
        int score = 0;
    };

    std::vector<ScoredMove> scored_moves;
    scored_moves.reserve(moves.size());

    for (const Move& move : moves) {
        int score = 0;
        if (is_valid_move_shape(pv_move) && same_move_identity(move, pv_move)) {
            score = 1'100'000;
        } else if (is_valid_move_shape(tt_move) && same_move_identity(move, tt_move)) {
            score = 1'000'000;
        } else {
            if (move.is_promotion()) {
                score += 90'000 + material_value(move.promotion);
            }
            if (move.is_capture()) {
                const int see_score = needs_see_for_loss_detection(board, move)
                    ? static_exchange_eval(board, move)
                    : immediate_capture_gain(board, move);
                if (see_score >= 0) {
                    score += 70'000 + see_score + mvv_lva_score(board, move);
                } else {
                    score += 5'000 + see_score + mvv_lva_score(board, move);
                }
            } else if (ply < kMaxPly) {
                if (same_move(move, killer_moves_[ply][0])) {
                    score += 30'000;
                } else if (same_move(move, killer_moves_[ply][1])) {
                    score += 29'000;
                }
                score += history_[color_index(side)][move.from][move.to];
            }
        }
        scored_moves.push_back(ScoredMove{move, score});
    }

    std::stable_sort(scored_moves.begin(), scored_moves.end(), [](const ScoredMove& lhs, const ScoredMove& rhs) {
        return lhs.score > rhs.score;
    });

    for (std::size_t index = 0; index < moves.size(); ++index) {
        moves[index] = scored_moves[index].move;
    }
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
