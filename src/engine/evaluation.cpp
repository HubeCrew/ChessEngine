#include "chess/engine/evaluation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>

namespace chess::engine {

namespace {

constexpr int kMaxPhase = 24;

constexpr std::array<int, 6> kPhaseWeights{
    0, 1, 1, 2, 4, 0,
};

constexpr std::array<int, 64> kPawnMg{{
      0,   0,   0,   0,   0,   0,   0,   0,
     50,  50,  50,  50,  50,  50,  50,  50,
     10,  10,  20,  30,  30,  20,  10,  10,
      5,   5,  10,  25,  25,  10,   5,   5,
      0,   0,   0,  20,  20,   0,   0,   0,
      5,  -5, -10,   0,   0, -10,  -5,   5,
      5,  10,  10, -20, -20,  10,  10,   5,
      0,   0,   0,   0,   0,   0,   0,   0,
}};

constexpr std::array<int, 64> kPawnEg{{
      0,   0,   0,   0,   0,   0,   0,   0,
     80,  80,  80,  80,  80,  80,  80,  80,
     50,  50,  50,  50,  50,  50,  50,  50,
     30,  30,  35,  40,  40,  35,  30,  30,
     15,  15,  20,  25,  25,  20,  15,  15,
     10,  10,  10,  15,  15,  10,  10,  10,
      5,   5,   5, -10, -10,   5,   5,   5,
      0,   0,   0,   0,   0,   0,   0,   0,
}};

constexpr std::array<int, 64> kKnightMg{{
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50,
}};

constexpr std::array<int, 64> kKnightEg{{
    -40, -30, -20, -20, -20, -20, -30, -40,
    -30, -10,   5,   5,   5,   5, -10, -30,
    -20,   5,  15,  20,  20,  15,   5, -20,
    -20,   5,  20,  25,  25,  20,   5, -20,
    -20,   5,  20,  25,  25,  20,   5, -20,
    -20,   5,  15,  20,  20,  15,   5, -20,
    -30, -10,   5,   5,   5,   5, -10, -30,
    -40, -30, -20, -20, -20, -20, -30, -40,
}};

constexpr std::array<int, 64> kBishopMg{{
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   0,  10,  15,  15,  10,   0, -10,
    -10,   5,   5,  15,  15,   5,   5, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10, -10, -10, -20,
}};

constexpr std::array<int, 64> kBishopEg{{
    -10,  -5,  -5,  -5,  -5,  -5,  -5, -10,
     -5,  10,   5,   5,   5,   5,  10,  -5,
     -5,  10,  15,  15,  15,  15,  10,  -5,
     -5,   5,  15,  20,  20,  15,   5,  -5,
     -5,   5,  15,  20,  20,  15,   5,  -5,
     -5,  10,  15,  15,  15,  15,  10,  -5,
     -5,  10,   5,   5,   5,   5,  10,  -5,
    -10,  -5,  -5,  -5,  -5,  -5,  -5, -10,
}};

constexpr std::array<int, 64> kRookMg{{
      0,   0,   0,   5,   5,   0,   0,   0,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
      5,  10,  10,  10,  10,  10,  10,   5,
      0,   0,   0,   5,   5,   0,   0,   0,
}};

constexpr std::array<int, 64> kRookEg{{
      0,   0,   5,  10,  10,   5,   0,   0,
      0,   0,   5,  10,  10,   5,   0,   0,
      0,   0,   5,  10,  10,   5,   0,   0,
      0,   0,   5,  10,  10,   5,   0,   0,
      0,   0,   5,  10,  10,   5,   0,   0,
      0,   0,   5,  10,  10,   5,   0,   0,
      5,  10,  10,  15,  15,  10,  10,   5,
      0,   0,   5,  10,  10,   5,   0,   0,
}};

constexpr std::array<int, 64> kQueenMg{{
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -10,   5,   5,   5,   5,   5,   0, -10,
      0,   0,   5,   5,   5,   5,   0,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20,
}};

constexpr std::array<int, 64> kQueenEg{{
    -10,  -5,  -5,   0,   0,  -5,  -5, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
     -5,   5,  10,  10,  10,  10,   5,  -5,
      0,   5,  10,  15,  15,  10,   5,   0,
      0,   5,  10,  15,  15,  10,   5,   0,
     -5,   5,  10,  10,  10,  10,   5,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,  -5,  -5,   0,   0,  -5,  -5, -10,
}};

constexpr std::array<int, 64> kKingMg{{
     20,  30,  10,   0,   0,  10,  30,  20,
     20,  20,   0,   0,   0,   0,  20,  20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
}};

constexpr std::array<int, 64> kKingEg{{
    -50, -30, -30, -30, -30, -30, -30, -50,
    -30, -10,   0,   0,   0,   0, -10, -30,
    -30,   0,  20,  30,  30,  20,   0, -30,
    -30,   0,  30,  40,  40,  30,   0, -30,
    -30,   0,  30,  40,  40,  30,   0, -30,
    -30,   0,  20,  30,  30,  20,   0, -30,
    -30, -10,   0,   0,   0,   0, -10, -30,
    -50, -30, -30, -30, -30, -30, -30, -50,
}};

int piece_index(PieceType type) {
    return static_cast<int>(type);
}

Square white_table_square(Square square, Color color) {
    if (color == Color::White) {
        return square;
    }
    return make_square(file_of(square), 7 - rank_of(square));
}

int table_value(PieceType type, Square square, Color color, bool endgame) {
    const Square table_square = white_table_square(square, color);
    switch (type) {
        case PieceType::Pawn:
            return endgame ? kPawnEg[table_square] : kPawnMg[table_square];
        case PieceType::Knight:
            return endgame ? kKnightEg[table_square] : kKnightMg[table_square];
        case PieceType::Bishop:
            return endgame ? kBishopEg[table_square] : kBishopMg[table_square];
        case PieceType::Rook:
            return endgame ? kRookEg[table_square] : kRookMg[table_square];
        case PieceType::Queen:
            return endgame ? kQueenEg[table_square] : kQueenMg[table_square];
        case PieceType::King:
            return endgame ? kKingEg[table_square] : kKingMg[table_square];
        case PieceType::None:
            return 0;
    }
    return 0;
}

bool has_own_piece(const Board& board, Square square, Color color) {
    return is_valid_square(square)
        && board.piece_at(square) != Piece::None
        && color_of(board.piece_at(square)) == color;
}

int knight_mobility(const Board& board, Square square, Color color) {
    constexpr std::array<std::pair<int, int>, 8> offsets{{
        {1, 2}, {2, 1}, {2, -1}, {1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2},
    }};
    int mobility = 0;
    for (const auto& [df, dr] : offsets) {
        const int file = file_of(square) + df;
        const int rank = rank_of(square) + dr;
        if (file >= 0 && file < 8 && rank >= 0 && rank < 8
            && !has_own_piece(board, make_square(file, rank), color)) {
            ++mobility;
        }
    }
    return mobility;
}

int slider_mobility(
    const Board& board,
    Square square,
    Color color,
    std::initializer_list<std::pair<int, int>> directions
) {
    int mobility = 0;
    for (const auto& [df, dr] : directions) {
        int file = file_of(square) + df;
        int rank = rank_of(square) + dr;
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const Square target = make_square(file, rank);
            if (has_own_piece(board, target, color)) {
                break;
            }
            ++mobility;
            if (board.piece_at(target) != Piece::None) {
                break;
            }
            file += df;
            rank += dr;
        }
    }
    return mobility;
}

int mobility_for_piece(const Board& board, Square square, Color color, PieceType type) {
    switch (type) {
        case PieceType::Knight:
            return knight_mobility(board, square, color) * 4;
        case PieceType::Bishop:
            return slider_mobility(board, square, color, {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}}) * 3;
        case PieceType::Rook:
            return slider_mobility(board, square, color, {{1, 0}, {0, -1}, {-1, 0}, {0, 1}}) * 2;
        case PieceType::Queen:
            return slider_mobility(
                board,
                square,
                color,
                {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}, {1, 0}, {0, -1}, {-1, 0}, {0, 1}}
            );
        case PieceType::Pawn:
        case PieceType::King:
        case PieceType::None:
            return 0;
    }
    return 0;
}

bool has_pawn_on_file(const Board& board, Color color, int file) {
    if (file < 0 || file > 7) {
        return false;
    }
    for (int rank = 0; rank < 8; ++rank) {
        if (board.piece_at(make_square(file, rank)) == make_piece(color, PieceType::Pawn)) {
            return true;
        }
    }
    return false;
}

int count_pawns_on_file(const Board& board, Color color, int file) {
    int count = 0;
    for (int rank = 0; rank < 8; ++rank) {
        if (board.piece_at(make_square(file, rank)) == make_piece(color, PieceType::Pawn)) {
            ++count;
        }
    }
    return count;
}

bool is_passed_pawn(const Board& board, Square square, Color color) {
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

int king_distance(Square from, Square to) {
    return std::max(std::abs(file_of(from) - file_of(to)), std::abs(rank_of(from) - rank_of(to)));
}

int non_pawn_material(const Board& board, Color color) {
    int total = 0;
    total += __builtin_popcountll(board.pieces(color, PieceType::Knight)) * material_value(PieceType::Knight);
    total += __builtin_popcountll(board.pieces(color, PieceType::Bishop)) * material_value(PieceType::Bishop);
    total += __builtin_popcountll(board.pieces(color, PieceType::Rook)) * material_value(PieceType::Rook);
    total += __builtin_popcountll(board.pieces(color, PieceType::Queen)) * material_value(PieceType::Queen);
    return total;
}

bool path_to_promotion_is_clear(const Board& board, Square square, Color color) {
    const int direction = color == Color::White ? 1 : -1;
    const int file = file_of(square);
    for (int rank = rank_of(square) + direction; rank >= 0 && rank < 8; rank += direction) {
        if (board.piece_at(make_square(file, rank)) != Piece::None) {
            return false;
        }
    }
    return true;
}

bool is_blocked_by_enemy(const Board& board, Square square, Color color) {
    const int direction = color == Color::White ? 1 : -1;
    const int rank = rank_of(square) + direction;
    if (rank < 0 || rank >= 8) {
        return false;
    }
    const Piece blocker = board.piece_at(make_square(file_of(square), rank));
    return blocker != Piece::None && color_of(blocker) == opposite(color);
}

bool is_protected_by_pawn(const Board& board, Square square, Color color) {
    const int support_rank = rank_of(square) + (color == Color::White ? -1 : 1);
    if (support_rank < 0 || support_rank >= 8) {
        return false;
    }
    for (const int file : {file_of(square) - 1, file_of(square) + 1}) {
        if (file >= 0 && file < 8
            && board.piece_at(make_square(file, support_rank)) == make_piece(color, PieceType::Pawn)) {
            return true;
        }
    }
    return false;
}

bool has_connected_pawn(const Board& board, Square square, Color color) {
    for (const int file : {file_of(square) - 1, file_of(square) + 1}) {
        if (file < 0 || file >= 8) {
            continue;
        }
        for (const int rank : {rank_of(square) - 1, rank_of(square), rank_of(square) + 1}) {
            if (rank >= 0 && rank < 8
                && board.piece_at(make_square(file, rank)) == make_piece(color, PieceType::Pawn)) {
                return true;
            }
        }
    }
    return false;
}

int passed_pawn_bonus(Square square, Color color, bool endgame) {
    const int advanced_rank = color == Color::White ? rank_of(square) : 7 - rank_of(square);
    constexpr std::array<int, 8> mg_bonus{{0, 0, 8, 16, 28, 45, 75, 0}};
    constexpr std::array<int, 8> eg_bonus{{0, 0, 15, 30, 55, 90, 140, 0}};
    return endgame ? eg_bonus[advanced_rank] : mg_bonus[advanced_rank];
}

int passed_pawn_dynamic_bonus(const Board& board, Square square, Color color, bool endgame) {
    if (!endgame) {
        return 0;
    }

    const int advanced_rank = color == Color::White ? rank_of(square) : 7 - rank_of(square);
    const bool clear_path = path_to_promotion_is_clear(board, square, color);
    int score = 0;

    if (clear_path) {
        score += 10 + advanced_rank * 8;
    } else if (is_blocked_by_enemy(board, square, color)) {
        score -= 18 + advanced_rank * 4;
    }

    if (is_protected_by_pawn(board, square, color)) {
        score += 8 + advanced_rank * 5;
    }
    if (has_connected_pawn(board, square, color)) {
        score += 8 + advanced_rank * 4;
    }

    const Square own_king = board.king_square(color);
    const Square enemy_king = board.king_square(opposite(color));
    if (is_valid_square(own_king) && is_valid_square(enemy_king)) {
        const Square promotion_square = make_square(file_of(square), color == Color::White ? 7 : 0);
        const int own_distance = king_distance(own_king, promotion_square);
        const int enemy_distance = king_distance(enemy_king, promotion_square);
        score += std::clamp(enemy_distance - own_distance, -3, 3) * 8;

        int pawn_steps = color == Color::White ? 7 - rank_of(square) : rank_of(square);
        if (board.side_to_move() != color) {
            ++pawn_steps;
        }
        if (clear_path && enemy_distance > pawn_steps) {
            const bool bare_king_race = non_pawn_material(board, opposite(color)) == 0;
            score += bare_king_race ? 80 + advanced_rank * 25 : 20 + advanced_rank * 8;
        }
    }

    return score;
}

int pawn_structure_score(const Board& board, Color color, bool endgame) {
    int score = 0;
    const Piece pawn = make_piece(color, PieceType::Pawn);

    for (int file = 0; file < 8; ++file) {
        const int count = count_pawns_on_file(board, color, file);
        if (count > 1) {
            score -= (count - 1) * (endgame ? 14 : 12);
        }
    }

    for (Square square = 0; square < kBoardSquareCount; ++square) {
        if (board.piece_at(square) != pawn) {
            continue;
        }
        const int file = file_of(square);
        if (!has_pawn_on_file(board, color, file - 1) && !has_pawn_on_file(board, color, file + 1)) {
            score -= endgame ? 12 : 10;
        }
        if (is_passed_pawn(board, square, color)) {
            score += passed_pawn_bonus(square, color, endgame);
            score += passed_pawn_dynamic_bonus(board, square, color, endgame);
        }
    }
    return score;
}

int king_shelter_score(const Board& board, Color color) {
    const Square king = board.king_square(color);
    if (!is_valid_square(king)) {
        return 0;
    }

    int score = 0;
    const int king_file = file_of(king);
    const int home_rank = color == Color::White ? 0 : 7;
    const int pawn_rank_1 = color == Color::White ? 1 : 6;
    const int pawn_rank_2 = color == Color::White ? 2 : 5;

    if (rank_of(king) == home_rank && (king_file <= 2 || king_file >= 5)) {
        score += 18;
    }

    for (int file = std::max(0, king_file - 1); file <= std::min(7, king_file + 1); ++file) {
        if (board.piece_at(make_square(file, pawn_rank_1)) == make_piece(color, PieceType::Pawn)) {
            score += 7;
        } else if (board.piece_at(make_square(file, pawn_rank_2)) == make_piece(color, PieceType::Pawn)) {
            score += 3;
        } else {
            score -= 6;
        }
    }
    return score;
}

int signed_score(Color color, int score) {
    return color == Color::White ? score : -score;
}

}  // namespace

int material_value(PieceType type) {
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

int evaluate_white_perspective(const Board& board) {
    int mg_score = 0;
    int eg_score = 0;
    int phase = 0;
    std::array<int, 2> bishops{0, 0};

    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const Piece piece = board.piece_at(square);
        if (piece == Piece::None) {
            continue;
        }

        const Color color = color_of(piece);
        const PieceType type = type_of(piece);
        const int material = material_value(type);
        const int mg_piece = material + table_value(type, square, color, false);
        const int eg_piece = material + table_value(type, square, color, true);
        mg_score += signed_score(color, mg_piece);
        eg_score += signed_score(color, eg_piece);
        phase += kPhaseWeights[piece_index(type)];

        if (type == PieceType::Bishop) {
            ++bishops[static_cast<int>(color)];
        }

        const int mobility = mobility_for_piece(board, square, color, type);
        mg_score += signed_score(color, mobility);
        eg_score += signed_score(color, mobility / 2);
    }

    if (bishops[static_cast<int>(Color::White)] >= 2) {
        mg_score += 30;
        eg_score += 40;
    }
    if (bishops[static_cast<int>(Color::Black)] >= 2) {
        mg_score -= 30;
        eg_score -= 40;
    }

    mg_score += pawn_structure_score(board, Color::White, false);
    mg_score -= pawn_structure_score(board, Color::Black, false);
    eg_score += pawn_structure_score(board, Color::White, true);
    eg_score -= pawn_structure_score(board, Color::Black, true);

    mg_score += king_shelter_score(board, Color::White);
    mg_score -= king_shelter_score(board, Color::Black);

    phase = std::min(phase, kMaxPhase);
    const int eg_phase = kMaxPhase - phase;
    return (mg_score * phase + eg_score * eg_phase) / kMaxPhase;
}

int evaluate(const Board& board) {
    const int score = evaluate_white_perspective(board);
    return board.side_to_move() == Color::White ? score : -score;
}

}  // namespace chess::engine
