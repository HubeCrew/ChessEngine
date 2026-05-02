#include "chess/engine/evaluation.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <utility>

namespace chess::engine {

namespace {

constexpr int kMaxPhase = 24;

constexpr std::array<int, 6> kPhaseWeights{
    0, 1, 1, 2, 4, 0,
};

constexpr std::array<std::pair<int, int>, 8> kKnightOffsets{{
    {1, 2}, {2, 1}, {2, -1}, {1, -2},
    {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2},
}};

constexpr std::array<std::pair<int, int>, 8> kKingOffsets{{
    {1, 1}, {1, 0}, {1, -1}, {0, -1},
    {-1, -1}, {-1, 0}, {-1, 1}, {0, 1},
}};

constexpr std::array<std::pair<int, int>, 4> kBishopDirections{{
    {1, 1}, {1, -1}, {-1, -1}, {-1, 1},
}};

constexpr std::array<std::pair<int, int>, 4> kRookDirections{{
    {1, 0}, {0, -1}, {-1, 0}, {0, 1},
}};

constexpr std::array<std::pair<int, int>, 8> kQueenDirections{{
    {1, 1}, {1, -1}, {-1, -1}, {-1, 1},
    {1, 0}, {0, -1}, {-1, 0}, {0, 1},
}};

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

bool has_piece(const Board& board, Square square, Color color, PieceType type) {
    return is_valid_square(square) && board.piece_at(square) == make_piece(color, type);
}

template <std::size_t Size>
Bitboard attacks_from_offsets(Square square, const std::array<std::pair<int, int>, Size>& offsets) {
    Bitboard attacks = 0;
    for (const auto& [df, dr] : offsets) {
        const int file = file_of(square) + df;
        const int rank = rank_of(square) + dr;
        if (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            attacks |= square_bb(make_square(file, rank));
        }
    }
    return attacks;
}

Bitboard pawn_attacks_from(Square square, Color color) {
    const int file = file_of(square);
    Bitboard attacks = 0;
    if (color == Color::White) {
        if (file > 0 && square + 7 < kBoardSquareCount) {
            attacks |= square_bb(square + 7);
        }
        if (file < 7 && square + 9 < kBoardSquareCount) {
            attacks |= square_bb(square + 9);
        }
    } else {
        if (file > 0 && square - 9 >= 0) {
            attacks |= square_bb(square - 9);
        }
        if (file < 7 && square - 7 >= 0) {
            attacks |= square_bb(square - 7);
        }
    }
    return attacks;
}

Bitboard knight_attacks_from(Square square) {
    return attacks_from_offsets(square, kKnightOffsets);
}

Bitboard king_attacks_from(Square square) {
    return attacks_from_offsets(square, kKingOffsets);
}

template <std::size_t Size>
Bitboard slider_attacks_from(
    const Board& board,
    Square from,
    const std::array<std::pair<int, int>, Size>& directions
) {
    Bitboard attacks = 0;
    for (const auto& [df, dr] : directions) {
        int file = file_of(from) + df;
        int rank = rank_of(from) + dr;
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const Square square = make_square(file, rank);
            attacks |= square_bb(square);
            if (board.piece_at(square) != Piece::None) {
                break;
            }
            file += df;
            rank += dr;
        }
    }
    return attacks;
}

Bitboard attacks_from_piece(const Board& board, Square from, Color color, PieceType type) {
    switch (type) {
        case PieceType::Pawn:
            return pawn_attacks_from(from, color);
        case PieceType::Knight:
            return knight_attacks_from(from);
        case PieceType::Bishop:
            return slider_attacks_from(board, from, kBishopDirections);
        case PieceType::Rook:
            return slider_attacks_from(board, from, kRookDirections);
        case PieceType::Queen:
            return slider_attacks_from(board, from, kQueenDirections);
        case PieceType::King:
            return king_attacks_from(from);
        case PieceType::None:
            return 0;
    }
    return 0;
}

struct AttackMap {
    std::array<Bitboard, 6> by_type{};
    Bitboard all = 0;
};

int mobility_for_piece(PieceType type, Bitboard attacks, Bitboard own_occupancy) {
    const int mobility = __builtin_popcountll(attacks & ~own_occupancy);
    switch (type) {
        case PieceType::Knight:
            return mobility * 4;
        case PieceType::Bishop:
            return mobility * 3;
        case PieceType::Rook:
            return mobility * 2;
        case PieceType::Queen:
            return mobility;
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

int castling_rights_for(Color color) {
    return color == Color::White ? WhiteKingSide | WhiteQueenSide : BlackKingSide | BlackQueenSide;
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

int shield_pawn_score(const Board& board, Color color, Square king, int file) {
    const int direction = color == Color::White ? 1 : -1;
    const int front_rank = rank_of(king) + direction;
    const int second_rank = rank_of(king) + 2 * direction;
    const int third_rank = rank_of(king) + 3 * direction;
    const bool king_file = file == file_of(king);

    if (front_rank >= 0 && front_rank < 8 && has_piece(board, make_square(file, front_rank), color, PieceType::Pawn)) {
        return king_file ? 10 : 8;
    }
    if (second_rank >= 0 && second_rank < 8 && has_piece(board, make_square(file, second_rank), color, PieceType::Pawn)) {
        return king_file ? 4 : 3;
    }
    if (third_rank >= 0 && third_rank < 8 && has_piece(board, make_square(file, third_rank), color, PieceType::Pawn)) {
        return king_file ? -3 : -2;
    }
    return king_file ? -14 : -10;
}

bool heavy_piece_on_file(const Board& board, Color color, int file) {
    for (int rank = 0; rank < 8; ++rank) {
        const Piece piece = board.piece_at(make_square(file, rank));
        if (piece != Piece::None && color_of(piece) == color
            && (type_of(piece) == PieceType::Rook || type_of(piece) == PieceType::Queen)) {
            return true;
        }
    }
    return false;
}

int king_shelter_score(const Board& board, Color color) {
    const Square king = board.king_square(color);
    if (!is_valid_square(king)) {
        return 0;
    }

    int score = 0;
    const int king_file = file_of(king);
    const int home_rank = color == Color::White ? 0 : 7;
    const Color enemy = opposite(color);

    if (rank_of(king) == home_rank && (king_file <= 2 || king_file >= 5)) {
        score += 22;
    } else if (rank_of(king) == home_rank && king_file >= 3 && king_file <= 4) {
        score -= (board.castling_rights() & castling_rights_for(color)) != 0 ? 12 : 32;
    } else {
        score -= 14;
    }

    for (int file = std::max(0, king_file - 1); file <= std::min(7, king_file + 1); ++file) {
        score += shield_pawn_score(board, color, king, file);

        const bool own_pawn_on_file = has_pawn_on_file(board, color, file);
        const bool enemy_pawn_on_file = has_pawn_on_file(board, enemy, file);
        const bool same_file = file == king_file;
        if (!own_pawn_on_file && !enemy_pawn_on_file) {
            score -= same_file ? 28 : 16;
        } else if (!own_pawn_on_file) {
            score -= same_file ? 16 : 9;
        }

        if (!own_pawn_on_file && heavy_piece_on_file(board, enemy, file)) {
            score -= same_file ? 28 : 18;
        }
    }
    return score;
}

struct KingRing {
    std::array<Square, 8> squares{};
    int count = 0;
};

KingRing king_ring(Square king) {
    KingRing ring;
    for (int df = -1; df <= 1; ++df) {
        for (int dr = -1; dr <= 1; ++dr) {
            if (df == 0 && dr == 0) {
                continue;
            }
            const int file = file_of(king) + df;
            const int rank = rank_of(king) + dr;
            if (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
                ring.squares[ring.count++] = make_square(file, rank);
            }
        }
    }
    return ring;
}

int king_attack_piece_weight(PieceType type) {
    switch (type) {
        case PieceType::Pawn:
            return 2;
        case PieceType::Knight:
            return 9;
        case PieceType::Bishop:
            return 7;
        case PieceType::Rook:
            return 11;
        case PieceType::Queen:
            return 18;
        case PieceType::King:
        case PieceType::None:
            return 0;
    }
    return 0;
}

int king_attack_penalty(const Board& board, Color defender, const AttackMap& attacker_map) {
    const Square king = board.king_square(defender);
    if (!is_valid_square(king)) {
        return 0;
    }

    const Color attacker = opposite(defender);
    const KingRing ring = king_ring(king);
    Bitboard ring_mask = 0;
    for (int index = 0; index < ring.count; ++index) {
        ring_mask |= square_bb(ring.squares[index]);
    }

    int attackers = 0;
    int attack_units = 0;
    bool queen_attacks = false;

    for (const PieceType type : {
             PieceType::Pawn,
             PieceType::Knight,
             PieceType::Bishop,
             PieceType::Rook,
             PieceType::Queen,
         }) {
        Bitboard pieces = board.pieces(attacker, type);
        while (pieces != 0) {
            const Square from = __builtin_ctzll(pieces);
            pieces &= pieces - 1;

            const int hits = __builtin_popcountll(attacks_from_piece(board, from, attacker, type) & ring_mask);
            if (hits > 0) {
                ++attackers;
                attack_units += king_attack_piece_weight(type) + hits * 2;
                queen_attacks = queen_attacks || type == PieceType::Queen;
            }
        }
    }

    int penalty = (attacker_map.all & square_bb(king)) != 0 ? 45 : 0;
    if (attackers >= 2) {
        penalty += attack_units + attackers * attackers * 4;
        if (queen_attacks) {
            penalty += 14;
        }
    } else if (attackers == 1) {
        penalty += attack_units / 3;
    }

    return penalty;
}

int king_safety_score(const Board& board, Color color, const AttackMap& enemy_attacks) {
    return king_shelter_score(board, color) - king_attack_penalty(board, color, enemy_attacks);
}

int loose_piece_bonus(PieceType type) {
    switch (type) {
        case PieceType::Pawn:
            return 8;
        case PieceType::Knight:
        case PieceType::Bishop:
            return 28;
        case PieceType::Rook:
            return 45;
        case PieceType::Queen:
            return 75;
        case PieceType::King:
        case PieceType::None:
            return 0;
    }
    return 0;
}

int pressure_bonus(PieceType type) {
    switch (type) {
        case PieceType::Pawn:
            return 1;
        case PieceType::Knight:
        case PieceType::Bishop:
            return 5;
        case PieceType::Rook:
            return 12;
        case PieceType::Queen:
            return 18;
        case PieceType::King:
        case PieceType::None:
            return 0;
    }
    return 0;
}

int pawn_threat_bonus(PieceType victim) {
    switch (victim) {
        case PieceType::Knight:
        case PieceType::Bishop:
            return 18;
        case PieceType::Rook:
            return 35;
        case PieceType::Queen:
            return 55;
        case PieceType::Pawn:
        case PieceType::King:
        case PieceType::None:
            return 0;
    }
    return 0;
}

int minor_threat_bonus(PieceType victim) {
    switch (victim) {
        case PieceType::Rook:
            return 14;
        case PieceType::Queen:
            return 24;
        case PieceType::Pawn:
        case PieceType::Knight:
        case PieceType::Bishop:
        case PieceType::King:
        case PieceType::None:
            return 0;
    }
    return 0;
}

int threat_score(const Board& board, Color color, const AttackMap& own_attacks, const AttackMap& enemy_attacks) {
    const Color enemy = opposite(color);
    int score = 0;

    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const Piece piece = board.piece_at(square);
        if (piece == Piece::None || color_of(piece) != enemy || type_of(piece) == PieceType::King) {
            continue;
        }

        const PieceType victim = type_of(piece);
        const Bitboard target = square_bb(square);
        if ((own_attacks.all & target) == 0) {
            continue;
        }

        const bool defended = (enemy_attacks.all & target) != 0;
        score += defended ? pressure_bonus(victim) : loose_piece_bonus(victim);

        if ((own_attacks.by_type[piece_index(PieceType::Pawn)] & target) != 0 && victim != PieceType::Pawn) {
            score += pawn_threat_bonus(victim);
        }

        const bool minor_attack = ((own_attacks.by_type[piece_index(PieceType::Knight)]
            | own_attacks.by_type[piece_index(PieceType::Bishop)]) & target) != 0;
        if (minor_attack && material_value(victim) > material_value(PieceType::Bishop)) {
            score += minor_threat_bonus(victim);
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
    std::array<AttackMap, 2> attack_maps{};

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

        const Bitboard attacks = attacks_from_piece(board, square, color, type);
        attack_maps[static_cast<int>(color)].by_type[piece_index(type)] |= attacks;
        attack_maps[static_cast<int>(color)].all |= attacks;

        const int mobility = mobility_for_piece(type, attacks, board.occupancy(color));
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

    const AttackMap& white_attacks = attack_maps[static_cast<int>(Color::White)];
    const AttackMap& black_attacks = attack_maps[static_cast<int>(Color::Black)];

    mg_score += king_safety_score(board, Color::White, black_attacks);
    mg_score -= king_safety_score(board, Color::Black, white_attacks);

    const int white_threats = threat_score(board, Color::White, white_attacks, black_attacks);
    const int black_threats = threat_score(board, Color::Black, black_attacks, white_attacks);
    mg_score += white_threats - black_threats;
    eg_score += (white_threats - black_threats) / 3;

    phase = std::min(phase, kMaxPhase);
    const int eg_phase = kMaxPhase - phase;
    return (mg_score * phase + eg_score * eg_phase) / kMaxPhase;
}

int evaluate(const Board& board) {
    const int score = evaluate_white_perspective(board);
    return board.side_to_move() == Color::White ? score : -score;
}

}  // namespace chess::engine
