#include "chess/engine/search.h"

#include <algorithm>
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

bool is_valid_move_shape(const Move& move) {
    return is_valid_square(move.from) && is_valid_square(move.to);
}

bool is_mate_score(int score) {
    return std::abs(score) > kMateScore - kMateWindow;
}

bool move_gives_check(Board& board, const Move& move) {
    const Color moving_side = board.side_to_move();
    const UndoState undo = board.make_move(move);
    const bool gives_check = board.in_check(opposite(moving_side));
    board.unmake_move(undo);
    return gives_check;
}

}  // namespace

Searcher::Searcher() {
    clear();
}

SearchResult Searcher::search(Board& board, const SearchLimits& limits) {
    nodes_ = 0;
    tt_hits_ = 0;
    use_deadline_ = limits.move_time.count() > 0;
    start_time_ = std::chrono::steady_clock::now();
    deadline_ = start_time_ + limits.move_time;
    tt_.new_search();

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

        int alpha = -kInfinity;
        int beta = kInfinity;
        int window = kAspirationWindow;
        if (depth > 1 && !is_mate_score(result.score_centipawns)) {
            alpha = std::max(-kInfinity, result.score_centipawns - window);
            beta = std::min(kInfinity, result.score_centipawns + window);
        }

        int score = 0;
        while (true) {
            score = negamax(board, depth, 0, alpha, beta);
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
            result.tt_hits = tt_hits_;
            result.principal_variation = extract_principal_variation(board, depth);
            if (!result.principal_variation.empty()) {
                result.best_move = result.principal_variation.front();
            }
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
}

int Searcher::negamax(Board& board, int depth, int ply, int alpha, int beta) {
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
        return quiescence(board, ply, alpha, beta);
    }

    MoveList moves = generate_legal_moves(board);
    if (moves.empty()) {
        return board.in_check(board.side_to_move()) ? -kMateScore + ply : 0;
    }
    order_moves(board, moves, tt_move, ply);

    Move best_move;
    int best_score = -kInfinity;
    int move_index = 0;
    for (const Move& move : moves) {
        const Color moving_side = board.side_to_move();
        const UndoState undo = board.make_move(move);
        int score = 0;
        if (move_index == 0) {
            score = -negamax(board, depth - 1, ply + 1, -beta, -alpha);
        } else {
            score = -negamax(board, depth - 1, ply + 1, -alpha - 1, -alpha);
            if (score > alpha && score < beta) {
                score = -negamax(board, depth - 1, ply + 1, -beta, -alpha);
            }
        }
        board.unmake_move(undo);
        ++move_index;

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta || should_stop()) {
            record_cutoff(move, depth, ply, moving_side);
            break;
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

int Searcher::quiescence(Board& board, int ply, int alpha, int beta) {
    if (should_stop()) {
        return evaluate(board);
    }
    ++nodes_;

    const bool in_check = board.in_check(board.side_to_move());
    if (!in_check) {
        const int stand_pat = evaluate(board);
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
        moves.erase(
            std::remove_if(moves.begin(), moves.end(), [](const Move& move) {
                return !move.is_capture() && !move.is_promotion();
            }),
            moves.end()
        );
    } else if (moves.empty()) {
        return -kMateScore + ply;
    }
    order_moves(board, moves, tt_move, ply);

    for (const Move& move : moves) {
        if (!in_check && needs_see_for_loss_detection(board, move)
            && static_exchange_eval(board, move) < 0 && !move_gives_check(board, move)) {
            continue;
        }

        const UndoState undo = board.make_move(move);
        const int score = -quiescence(board, ply + 1, -beta, -alpha);
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

    struct ScoredMove {
        Move move;
        int score = 0;
    };

    std::vector<ScoredMove> scored_moves;
    scored_moves.reserve(moves.size());

    for (const Move& move : moves) {
        int score = 0;
        if (is_valid_move_shape(tt_move)
            && move.from == tt_move.from
            && move.to == tt_move.to
            && move.promotion == tt_move.promotion) {
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

void Searcher::record_cutoff(const Move& move, int depth, int ply, Color side_to_move) {
    if (move.is_capture() || move.is_promotion() || ply >= kMaxPly) {
        return;
    }

    if (!same_move(move, killer_moves_[ply][0])) {
        killer_moves_[ply][1] = killer_moves_[ply][0];
        killer_moves_[ply][0] = move;
    }

    int& history_score = history_[color_index(side_to_move)][move.from][move.to];
    history_score += depth * depth;
    history_score = std::min(history_score, 1'000'000);
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
