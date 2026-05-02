#include "chess/engine/search.h"

#include <algorithm>
#include <array>
#include <limits>

#include "chess/core/movegen.h"

namespace chess::engine {

namespace {

constexpr int kInfinity = 1'000'000;
constexpr int kMateScore = 900'000;

int piece_value(PieceType type) {
    switch (type) {
        case PieceType::Pawn:
            return 100;
        case PieceType::Knight:
            return 320;
        case PieceType::Bishop:
            return 330;
        case PieceType::Rook:
            return 500;
        case PieceType::Queen:
            return 900;
        case PieceType::King:
        case PieceType::None:
            return 0;
    }
    return 0;
}

void order_moves(MoveList& moves) {
    std::stable_sort(moves.begin(), moves.end(), [](const Move& lhs, const Move& rhs) {
        const int lhs_score = (lhs.is_capture() ? 10 : 0) + (lhs.is_promotion() ? 20 : 0);
        const int rhs_score = (rhs.is_capture() ? 10 : 0) + (rhs.is_promotion() ? 20 : 0);
        return lhs_score > rhs_score;
    });
}

}  // namespace

SearchResult Searcher::search(Board& board, const SearchLimits& limits) {
    nodes_ = 0;
    use_deadline_ = limits.move_time.count() > 0;
    deadline_ = std::chrono::steady_clock::now() + limits.move_time;

    SearchResult result;
    MoveList moves = generate_legal_moves(board);
    if (moves.empty()) {
        result.best_move = Move{};
        result.score_centipawns = board.in_check(board.side_to_move()) ? -kMateScore : 0;
        return result;
    }

    order_moves(moves);
    result.best_move = moves.front();
    result.score_centipawns = -kInfinity;

    const int max_depth = std::max(1, limits.depth);
    for (int depth = 1; depth <= max_depth; ++depth) {
        if (should_stop()) {
            break;
        }

        Move best_this_depth = result.best_move;
        int best_score = -kInfinity;
        for (const Move& move : moves) {
            const UndoState undo = board.make_move(move);
            const int score = -negamax(board, depth - 1, -kInfinity, kInfinity);
            board.unmake_move(undo);
            if (should_stop()) {
                break;
            }
            if (score > best_score) {
                best_score = score;
                best_this_depth = move;
            }
        }

        if (!should_stop()) {
            result.best_move = best_this_depth;
            result.score_centipawns = best_score;
            result.depth = depth;
            result.nodes = nodes_;
        }
    }

    return result;
}

int Searcher::negamax(Board& board, int depth, int alpha, int beta) {
    if (should_stop()) {
        return evaluate(board);
    }
    ++nodes_;

    if (depth == 0) {
        return quiescence(board, alpha, beta);
    }

    MoveList moves = generate_legal_moves(board);
    if (moves.empty()) {
        return board.in_check(board.side_to_move()) ? -kMateScore - depth : 0;
    }
    order_moves(moves);

    int best = -kInfinity;
    for (const Move& move : moves) {
        const UndoState undo = board.make_move(move);
        const int score = -negamax(board, depth - 1, -beta, -alpha);
        board.unmake_move(undo);

        best = std::max(best, score);
        alpha = std::max(alpha, score);
        if (alpha >= beta || should_stop()) {
            break;
        }
    }
    return best;
}

int Searcher::quiescence(Board& board, int alpha, int beta) {
    if (should_stop()) {
        return evaluate(board);
    }
    ++nodes_;

    const int stand_pat = evaluate(board);
    if (stand_pat >= beta) {
        return beta;
    }
    alpha = std::max(alpha, stand_pat);

    MoveList moves = generate_legal_moves(board);
    moves.erase(
        std::remove_if(moves.begin(), moves.end(), [](const Move& move) {
            return !move.is_capture() && !move.is_promotion();
        }),
        moves.end()
    );
    order_moves(moves);

    for (const Move& move : moves) {
        const UndoState undo = board.make_move(move);
        const int score = -quiescence(board, -beta, -alpha);
        board.unmake_move(undo);
        if (score >= beta) {
            return beta;
        }
        alpha = std::max(alpha, score);
    }
    return alpha;
}

int Searcher::evaluate(const Board& board) const {
    int white_score = 0;
    int black_score = 0;
    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const Piece piece = board.piece_at(square);
        if (piece == Piece::None) {
            continue;
        }
        const int value = piece_value(type_of(piece));
        if (color_of(piece) == Color::White) {
            white_score += value;
        } else {
            black_score += value;
        }
    }

    const int score = white_score - black_score;
    return board.side_to_move() == Color::White ? score : -score;
}

bool Searcher::should_stop() const {
    return use_deadline_ && std::chrono::steady_clock::now() >= deadline_;
}

}  // namespace chess::engine

