#include "chess/core/game_status.h"

#include <array>

#include "chess/core/movegen.h"

namespace chess {

namespace {

constexpr int color_index(Color color) {
    return static_cast<int>(color);
}

constexpr int type_index(PieceType type) {
    return static_cast<int>(type);
}

int popcount(Bitboard value) {
    return __builtin_popcountll(value);
}

bool has_threefold_repetition(const Board& board, const std::vector<std::uint64_t>& repetition_history) {
    int occurrences = 0;
    for (const std::uint64_t hash : repetition_history) {
        if (hash == board.hash_key()) {
            ++occurrences;
        }
    }
    return occurrences >= 3;
}

}  // namespace

bool has_insufficient_material(const Board& board) {
    std::array<std::array<int, 6>, 2> counts{};
    int non_king_pieces = 0;
    int bishop_light_squares = 0;
    int bishop_dark_squares = 0;

    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const Piece piece = board.piece_at(square);
        if (piece == Piece::None) {
            continue;
        }

        const PieceType type = type_of(piece);
        ++counts[color_index(color_of(piece))][type_index(type)];
        if (type != PieceType::King) {
            ++non_king_pieces;
        }
        if (type == PieceType::Bishop) {
            if (((file_of(square) + rank_of(square)) & 1) == 0) {
                ++bishop_dark_squares;
            } else {
                ++bishop_light_squares;
            }
        }
    }

    if (non_king_pieces == 0) {
        return true;
    }

    if (popcount(board.pieces(Color::White, PieceType::Pawn) | board.pieces(Color::Black, PieceType::Pawn)) > 0
        || popcount(board.pieces(Color::White, PieceType::Rook) | board.pieces(Color::Black, PieceType::Rook)) > 0
        || popcount(board.pieces(Color::White, PieceType::Queen) | board.pieces(Color::Black, PieceType::Queen)) > 0) {
        return false;
    }

    const int white_minor = counts[color_index(Color::White)][type_index(PieceType::Bishop)]
        + counts[color_index(Color::White)][type_index(PieceType::Knight)];
    const int black_minor = counts[color_index(Color::Black)][type_index(PieceType::Bishop)]
        + counts[color_index(Color::Black)][type_index(PieceType::Knight)];

    if (non_king_pieces == 1 && (white_minor + black_minor) == 1) {
        return true;
    }

    const bool only_bishops = counts[color_index(Color::White)][type_index(PieceType::Knight)] == 0
        && counts[color_index(Color::Black)][type_index(PieceType::Knight)] == 0
        && (white_minor + black_minor) == non_king_pieces;
    if (only_bishops && (bishop_light_squares == 0 || bishop_dark_squares == 0)) {
        return true;
    }

    return false;
}

GameStatus adjudicate(Board& board, const std::vector<std::uint64_t>& repetition_history) {
    const MoveList legal_moves = generate_legal_moves(board);
    if (legal_moves.empty()) {
        if (board.in_check(board.side_to_move())) {
            return GameStatus{
                board.side_to_move() == Color::White ? GameOutcome::BlackWin : GameOutcome::WhiteWin,
                GameReason::Checkmate,
                0,
            };
        }
        return GameStatus{GameOutcome::Draw, GameReason::Stalemate, 0};
    }

    if (board.halfmove_clock() >= 100) {
        return GameStatus{GameOutcome::Draw, GameReason::FiftyMoveRule, static_cast<int>(legal_moves.size())};
    }

    if (has_threefold_repetition(board, repetition_history)) {
        return GameStatus{GameOutcome::Draw, GameReason::ThreefoldRepetition, static_cast<int>(legal_moves.size())};
    }

    if (has_insufficient_material(board)) {
        return GameStatus{GameOutcome::Draw, GameReason::InsufficientMaterial, static_cast<int>(legal_moves.size())};
    }

    return GameStatus{GameOutcome::Ongoing, GameReason::None, static_cast<int>(legal_moves.size())};
}

std::string_view to_string(GameOutcome outcome) {
    switch (outcome) {
        case GameOutcome::Ongoing:
            return "ongoing";
        case GameOutcome::WhiteWin:
            return "white_win";
        case GameOutcome::BlackWin:
            return "black_win";
        case GameOutcome::Draw:
            return "draw";
    }
    return "ongoing";
}

std::string_view to_string(GameReason reason) {
    switch (reason) {
        case GameReason::None:
            return "none";
        case GameReason::Checkmate:
            return "checkmate";
        case GameReason::Stalemate:
            return "stalemate";
        case GameReason::FiftyMoveRule:
            return "fifty_move_rule";
        case GameReason::ThreefoldRepetition:
            return "threefold_repetition";
        case GameReason::InsufficientMaterial:
            return "insufficient_material";
    }
    return "none";
}

}  // namespace chess
