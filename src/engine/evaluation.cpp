#include "chess/engine/evaluation.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <utility>

#include "chess/core/attacks.h"

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

bool has_piece(const Board& board, Square square, Color color, PieceType type) {
    return is_valid_square(square) && board.piece_at(square) == make_piece(color, type);
}

Bitboard attacks_from_piece(const Board& board, Square from, Color color, PieceType type) {
    return attacks::piece_attacks(type, color, from, board.occupancy());
}

struct AttackMap {
    std::array<Bitboard, 6> by_type{};
    std::array<Bitboard, kBoardSquareCount> by_square{};
    Bitboard all = 0;
};

int signed_score(Color color, int score) {
    return color == Color::White ? score : -score;
}

struct EvalTracePair {
    EvalTrace mg;
    EvalTrace eg;
};

void add_score(int& component, Color color, int score) {
    component += signed_score(color, score);
}

int blended_score(int mg_score, int eg_score, int phase) {
    const int eg_phase = kMaxPhase - phase;
    return (mg_score * phase + eg_score * eg_phase) / kMaxPhase;
}

EvalTrace blend_trace(const EvalTracePair& pair, int phase) {
    EvalTrace trace;
    trace.material = blended_score(pair.mg.material, pair.eg.material, phase);
    trace.piece_square = blended_score(pair.mg.piece_square, pair.eg.piece_square, phase);
    trace.mobility = blended_score(pair.mg.mobility, pair.eg.mobility, phase);
    trace.safe_mobility = blended_score(pair.mg.safe_mobility, pair.eg.safe_mobility, phase);
    trace.king_safety = blended_score(pair.mg.king_safety, pair.eg.king_safety, phase);
    trace.threats = blended_score(pair.mg.threats, pair.eg.threats, phase);
    trace.pawn_structure = blended_score(pair.mg.pawn_structure, pair.eg.pawn_structure, phase);
    trace.outposts = blended_score(pair.mg.outposts, pair.eg.outposts, phase);
    trace.rook_files = blended_score(pair.mg.rook_files, pair.eg.rook_files, phase);
    trace.space = blended_score(pair.mg.space, pair.eg.space, phase);
    trace.center_control = blended_score(pair.mg.center_control, pair.eg.center_control, phase);
    trace.bishop_quality = blended_score(pair.mg.bishop_quality, pair.eg.bishop_quality, phase);
    trace.pawn_dynamics = blended_score(pair.mg.pawn_dynamics, pair.eg.pawn_dynamics, phase);
    trace.development = blended_score(pair.mg.development, pair.eg.development, phase);
    trace.trade_context = blended_score(pair.mg.trade_context, pair.eg.trade_context, phase);
    trace.total = trace.material
        + trace.piece_square
        + trace.mobility
        + trace.safe_mobility
        + trace.king_safety
        + trace.threats
        + trace.pawn_structure
        + trace.outposts
        + trace.rook_files
        + trace.space
        + trace.center_control
        + trace.bishop_quality
        + trace.pawn_dynamics
        + trace.development
        + trace.trade_context;
    return trace;
}

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

Bitboard pawn_attack_map(const Board& board, Color color) {
    Bitboard attacks = 0;
    Bitboard pawns = board.pieces(color, PieceType::Pawn);
    while (pawns != 0) {
        const Square square = __builtin_ctzll(pawns);
        pawns &= pawns - 1;
        attacks |= attacks::pawn_attacks(color, square);
    }
    return attacks;
}

int safe_mobility_for_piece(PieceType type, Bitboard attacks, Bitboard own_occupancy, Bitboard enemy_pawn_attacks) {
    const int mobility = __builtin_popcountll(attacks & ~own_occupancy & ~enemy_pawn_attacks);
    switch (type) {
        case PieceType::Knight:
            return mobility * 2;
        case PieceType::Bishop:
            return mobility * 2;
        case PieceType::Rook:
            return mobility;
        case PieceType::Queen:
            return mobility / 2;
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
    constexpr Bitboard kAFile = 0x0101010101010101ULL;
    return (board.pieces(color, PieceType::Pawn) & (kAFile << file)) != 0;
}

int count_pawns_on_file(const Board& board, Color color, int file) {
    if (file < 0 || file > 7) {
        return 0;
    }
    constexpr Bitboard kAFile = 0x0101010101010101ULL;
    return __builtin_popcountll(board.pieces(color, PieceType::Pawn) & (kAFile << file));
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
    return (attacks::pawn_attacks(opposite(color), square) & board.pieces(color, PieceType::Pawn)) != 0;
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

            const int hits = __builtin_popcountll(attacker_map.by_square[from] & ring_mask);
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

int least_attacker_value(const AttackMap& attacks, Bitboard target) {
    for (const PieceType attacker : {
             PieceType::Pawn,
             PieceType::Knight,
             PieceType::Bishop,
             PieceType::Rook,
             PieceType::Queen,
             PieceType::King,
         }) {
        if ((attacks.by_type[piece_index(attacker)] & target) != 0) {
            return material_value(attacker);
        }
    }
    return 0;
}

int lower_value_attacker_pressure(PieceType victim, int attacker_value, bool defended) {
    const int victim_value = material_value(victim);
    if (attacker_value <= 0 || attacker_value >= victim_value) {
        return 0;
    }

    int bonus = 0;
    switch (victim) {
        case PieceType::Queen:
            bonus = attacker_value <= material_value(PieceType::Pawn) ? 85 : 65;
            break;
        case PieceType::Rook:
            bonus = attacker_value <= material_value(PieceType::Pawn) ? 55 : 35;
            break;
        case PieceType::Knight:
        case PieceType::Bishop:
            bonus = attacker_value <= material_value(PieceType::Pawn) ? 16 : 0;
            break;
        case PieceType::Pawn:
        case PieceType::King:
        case PieceType::None:
            return 0;
    }

    return defended ? bonus / 2 : bonus;
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

        score += lower_value_attacker_pressure(victim, least_attacker_value(own_attacks, target), defended);
    }

    return score;
}

bool is_light_square(Square square) {
    return ((file_of(square) + rank_of(square)) & 1) == 0;
}

bool file_has_pawn_ahead(const Board& board, Color color, int file, int rank) {
    const int direction = color == Color::White ? 1 : -1;
    for (int scan_rank = rank + direction; scan_rank >= 0 && scan_rank < 8; scan_rank += direction) {
        if (board.piece_at(make_square(file, scan_rank)) == make_piece(color, PieceType::Pawn)) {
            return true;
        }
    }
    return false;
}

bool enemy_pawn_can_challenge(const Board& board, Square square, Color color) {
    const Color enemy = opposite(color);
    const int enemy_direction = enemy == Color::White ? 1 : -1;
    for (const int file : {file_of(square) - 1, file_of(square) + 1}) {
        if (file < 0 || file >= 8) {
            continue;
        }
        for (int rank = rank_of(square) - enemy_direction; rank >= 0 && rank < 8; rank -= enemy_direction) {
            if (board.piece_at(make_square(file, rank)) == make_piece(enemy, PieceType::Pawn)) {
                return true;
            }
        }
    }
    return false;
}

int outpost_score(const Board& board, Color color, Bitboard enemy_pawn_attacks) {
    int score = 0;
    Bitboard knights = board.pieces(color, PieceType::Knight);
    while (knights != 0) {
        const Square square = __builtin_ctzll(knights);
        knights &= knights - 1;

        const int advanced_rank = color == Color::White ? rank_of(square) : 7 - rank_of(square);
        if (advanced_rank < 3 || !is_protected_by_pawn(board, square, color)
            || (enemy_pawn_attacks & square_bb(square)) != 0 || enemy_pawn_can_challenge(board, square, color)) {
            continue;
        }

        const bool central = file_of(square) >= 2 && file_of(square) <= 5;
        score += 18 + advanced_rank * 4 + (central ? 10 : 0);
    }
    return score;
}

int rook_file_score(const Board& board, Color color) {
    const Color enemy = opposite(color);
    int score = 0;
    Bitboard rooks = board.pieces(color, PieceType::Rook);
    while (rooks != 0) {
        const Square square = __builtin_ctzll(rooks);
        rooks &= rooks - 1;

        const int file = file_of(square);
        const bool own_pawn = has_pawn_on_file(board, color, file);
        const bool enemy_pawn = has_pawn_on_file(board, enemy, file);
        if (!own_pawn && !enemy_pawn) {
            score += 24;
        } else if (!own_pawn) {
            score += 12;
        }

        const int rank = color == Color::White ? 6 : 1;
        if (rank_of(square) == rank) {
            score += 22;
        }

        const Square enemy_king = board.king_square(enemy);
        if (is_valid_square(enemy_king) && file_of(enemy_king) == file && !own_pawn) {
            score += enemy_pawn ? 8 : 16;
        }
    }
    return score;
}

int center_control_score(const Board& board, Color color, const AttackMap& attack_map) {
    constexpr std::array<Square, 4> center{
        make_square(3, 3), make_square(4, 3), make_square(3, 4), make_square(4, 4),
    };
    constexpr std::array<Square, 12> extended_center{
        make_square(2, 2), make_square(3, 2), make_square(4, 2), make_square(5, 2),
        make_square(2, 3), make_square(5, 3), make_square(2, 4), make_square(5, 4),
        make_square(2, 5), make_square(3, 5), make_square(4, 5), make_square(5, 5),
    };

    int score = 0;
    for (const Square square : center) {
        const Bitboard bit = square_bb(square);
        if ((attack_map.all & bit) != 0) {
            score += 6;
        }
        if ((attack_map.by_type[piece_index(PieceType::Pawn)] & bit) != 0) {
            score += 4;
        }
        if (board.piece_at(square) == make_piece(color, PieceType::Pawn)) {
            score += 10;
        }
    }
    for (const Square square : extended_center) {
        if ((attack_map.all & square_bb(square)) != 0) {
            score += 2;
        }
    }
    return score;
}

int space_score(const Board& board, Color color, const AttackMap& attack_map, Bitboard enemy_pawn_attacks) {
    int score = 0;
    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const int advanced_rank = color == Color::White ? rank_of(square) : 7 - rank_of(square);
        if (advanced_rank < 4) {
            continue;
        }
        const Bitboard bit = square_bb(square);
        if ((attack_map.all & bit) == 0 || (enemy_pawn_attacks & bit) != 0) {
            continue;
        }
        const Piece occupant = board.piece_at(square);
        if (occupant != Piece::None && color_of(occupant) == color) {
            continue;
        }
        score += file_of(square) >= 2 && file_of(square) <= 5 ? 3 : 1;
    }
    return std::min(score, 48);
}

bool is_backward_pawn(const Board& board, Square square, Color color, Bitboard enemy_pawn_attacks) {
    if (is_passed_pawn(board, square, color) || is_protected_by_pawn(board, square, color)) {
        return false;
    }

    const int direction = color == Color::White ? 1 : -1;
    const int front_rank = rank_of(square) + direction;
    if (front_rank < 0 || front_rank >= 8) {
        return false;
    }
    const Square front = make_square(file_of(square), front_rank);
    if ((enemy_pawn_attacks & square_bb(front)) == 0) {
        return false;
    }

    for (const int file : {file_of(square) - 1, file_of(square) + 1}) {
        if (file < 0 || file >= 8) {
            continue;
        }
        if (file_has_pawn_ahead(board, color, file, rank_of(square) - direction)) {
            return false;
        }
    }
    return true;
}

bool is_candidate_passed_pawn(const Board& board, Square square, Color color) {
    if (is_passed_pawn(board, square, color) || is_blocked_by_enemy(board, square, color)) {
        return false;
    }
    const Color enemy = opposite(color);
    const int direction = color == Color::White ? 1 : -1;
    int blockers = 0;
    for (int file = std::max(0, file_of(square) - 1); file <= std::min(7, file_of(square) + 1); ++file) {
        for (int rank = rank_of(square) + direction; rank >= 0 && rank < 8; rank += direction) {
            if (board.piece_at(make_square(file, rank)) == make_piece(enemy, PieceType::Pawn)) {
                ++blockers;
            }
        }
    }
    return blockers <= 1 && (is_protected_by_pawn(board, square, color) || has_connected_pawn(board, square, color));
}

int pawn_dynamics_score(const Board& board, Color color, Bitboard enemy_pawn_attacks) {
    int score = 0;
    Bitboard pawns = board.pieces(color, PieceType::Pawn);
    while (pawns != 0) {
        const Square square = __builtin_ctzll(pawns);
        pawns &= pawns - 1;
        const int advanced_rank = color == Color::White ? rank_of(square) : 7 - rank_of(square);

        if (is_backward_pawn(board, square, color, enemy_pawn_attacks)) {
            score -= 12 + advanced_rank * 2;
        }
        if (is_candidate_passed_pawn(board, square, color)) {
            score += 10 + advanced_rank * 4;
        }
        if (is_protected_by_pawn(board, square, color)) {
            score += 4;
        }

        const Bitboard pawn_attacks = attacks::pawn_attacks(color, square);
        Bitboard enemy_pawns = pawn_attacks & board.pieces(opposite(color), PieceType::Pawn);
        if (enemy_pawns != 0) {
            score += advanced_rank >= 3 ? 8 : 4;
        }
    }
    return score;
}

int bishop_quality_score(const Board& board, Color color, const AttackMap& attack_map) {
    int score = 0;
    Bitboard bishops = board.pieces(color, PieceType::Bishop);
    while (bishops != 0) {
        const Square bishop = __builtin_ctzll(bishops);
        bishops &= bishops - 1;

        int same_color_pawns = 0;
        Bitboard pawns = board.pieces(color, PieceType::Pawn);
        while (pawns != 0) {
            const Square pawn = __builtin_ctzll(pawns);
            pawns &= pawns - 1;
            if (is_light_square(pawn) == is_light_square(bishop)) {
                ++same_color_pawns;
            }
        }

        const int mobility = __builtin_popcountll(attack_map.by_square[bishop] & ~board.occupancy(color));
        if (same_color_pawns >= 4 && mobility <= 5) {
            score -= 8 + (same_color_pawns - 3) * 4;
        }
        if (mobility >= 9) {
            score += 8;
        }
    }
    return score;
}

int development_score(const Board& board, Color color) {
    const int home_rank = color == Color::White ? 0 : 7;
    int score = 0;
    if (has_piece(board, make_square(1, home_rank), color, PieceType::Knight)) {
        score -= 8;
    }
    if (has_piece(board, make_square(6, home_rank), color, PieceType::Knight)) {
        score -= 8;
    }
    if (has_piece(board, make_square(2, home_rank), color, PieceType::Bishop)) {
        score -= 6;
    }
    if (has_piece(board, make_square(5, home_rank), color, PieceType::Bishop)) {
        score -= 6;
    }
    const Square king = board.king_square(color);
    if (is_valid_square(king) && rank_of(king) == home_rank && file_of(king) == 4
        && (board.castling_rights() & castling_rights_for(color)) == 0) {
        score -= 14;
    }
    return score;
}

int battery_score(const Board& board, Color color, const AttackMap& attack_map) {
    const Color enemy = opposite(color);
    int score = 0;
    Bitboard queens = board.pieces(color, PieceType::Queen);
    while (queens != 0) {
        const Square queen = __builtin_ctzll(queens);
        queens &= queens - 1;
        Bitboard rooks = board.pieces(color, PieceType::Rook);
        while (rooks != 0) {
            const Square rook = __builtin_ctzll(rooks);
            rooks &= rooks - 1;
            if (file_of(queen) == file_of(rook) || rank_of(queen) == rank_of(rook)) {
                const Bitboard between = attacks::between(queen, rook);
                if ((between & board.occupancy()) == 0) {
                    score += 10;
                }
            }
        }
    }

    const Square enemy_king = board.king_square(enemy);
    if (is_valid_square(enemy_king)) {
        const Bitboard king_zone = attacks::king_attacks(enemy_king) | square_bb(enemy_king);
        if ((attack_map.by_type[piece_index(PieceType::Queen)] & king_zone) != 0
            && (attack_map.by_type[piece_index(PieceType::Rook)] & king_zone) != 0) {
            score += 18;
        }
    }
    return score;
}

int trade_context_score(
    const Board& board,
    Color color,
    int material_balance,
    int total_non_pawn,
    int own_king_attack,
    int enemy_king_attack,
    int own_space,
    int enemy_space
) {
    const Color enemy = opposite(color);
    const int simplification = std::clamp((3600 - total_non_pawn) / 300, 0, 8);
    const int complexity = std::clamp((total_non_pawn - 1200) / 300, 0, 8);
    const int attack_balance = own_king_attack - enemy_king_attack;
    const bool own_queen = board.pieces(color, PieceType::Queen) != 0;
    const bool enemy_queen = board.pieces(enemy, PieceType::Queen) != 0;

    int score = 0;
    if (material_balance >= 150) {
        score += std::min(40, (material_balance / 100) * simplification);
    } else if (material_balance <= -150 && attack_balance > 20) {
        score += std::min(35, attack_balance / 4 + complexity * 2);
    }

    if (attack_balance > 25 && own_queen && enemy_queen) {
        score += std::min(42, attack_balance / 3 + complexity);
    }
    if (own_space + 10 < enemy_space && material_balance >= -50) {
        score += 8 + std::min(20, (enemy_space - own_space) / 2);
    }
    return score;
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

EvalTrace evaluate_trace_white_perspective(const Board& board) {
    EvalTracePair trace_pair;
    int phase = 0;
    std::array<int, 2> bishops{0, 0};
    std::array<int, 2> material_by_color{0, 0};
    std::array<AttackMap, 2> attack_maps{};
    const std::array<Bitboard, 2> pawn_attacks{
        pawn_attack_map(board, Color::White),
        pawn_attack_map(board, Color::Black),
    };

    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const Piece piece = board.piece_at(square);
        if (piece == Piece::None) {
            continue;
        }

        const Color color = color_of(piece);
        const PieceType type = type_of(piece);
        const int material = material_value(type);
        material_by_color[static_cast<int>(color)] += material;
        add_score(trace_pair.mg.material, color, material);
        add_score(trace_pair.eg.material, color, material);
        add_score(trace_pair.mg.piece_square, color, table_value(type, square, color, false));
        add_score(trace_pair.eg.piece_square, color, table_value(type, square, color, true));
        phase += kPhaseWeights[piece_index(type)];

        if (type == PieceType::Bishop) {
            ++bishops[static_cast<int>(color)];
        }

        const Bitboard attacks = attacks_from_piece(board, square, color, type);
        attack_maps[static_cast<int>(color)].by_type[piece_index(type)] |= attacks;
        attack_maps[static_cast<int>(color)].by_square[square] = attacks;
        attack_maps[static_cast<int>(color)].all |= attacks;

        const int mobility = mobility_for_piece(type, attacks, board.occupancy(color));
        add_score(trace_pair.mg.mobility, color, mobility);
        add_score(trace_pair.eg.mobility, color, mobility / 2);

        const int safe_mobility = safe_mobility_for_piece(
            type,
            attacks,
            board.occupancy(color),
            pawn_attacks[static_cast<int>(opposite(color))]
        );
        add_score(trace_pair.mg.safe_mobility, color, safe_mobility);
        add_score(trace_pair.eg.safe_mobility, color, safe_mobility / 2);
    }

    if (bishops[static_cast<int>(Color::White)] >= 2) {
        trace_pair.mg.bishop_quality += 30;
        trace_pair.eg.bishop_quality += 40;
    }
    if (bishops[static_cast<int>(Color::Black)] >= 2) {
        trace_pair.mg.bishop_quality -= 30;
        trace_pair.eg.bishop_quality -= 40;
    }

    trace_pair.mg.pawn_structure += pawn_structure_score(board, Color::White, false);
    trace_pair.mg.pawn_structure -= pawn_structure_score(board, Color::Black, false);
    trace_pair.eg.pawn_structure += pawn_structure_score(board, Color::White, true);
    trace_pair.eg.pawn_structure -= pawn_structure_score(board, Color::Black, true);

    const AttackMap& white_attacks = attack_maps[static_cast<int>(Color::White)];
    const AttackMap& black_attacks = attack_maps[static_cast<int>(Color::Black)];

    const int black_attack_on_white_king = king_attack_penalty(board, Color::White, black_attacks);
    const int white_attack_on_black_king = king_attack_penalty(board, Color::Black, white_attacks);
    trace_pair.mg.king_safety += king_shelter_score(board, Color::White) - black_attack_on_white_king;
    trace_pair.mg.king_safety -= king_shelter_score(board, Color::Black) - white_attack_on_black_king;

    const int white_threats = threat_score(board, Color::White, white_attacks, black_attacks);
    const int black_threats = threat_score(board, Color::Black, black_attacks, white_attacks);
    const int white_battery = battery_score(board, Color::White, white_attacks);
    const int black_battery = battery_score(board, Color::Black, black_attacks);
    trace_pair.mg.threats += white_threats - black_threats + white_battery - black_battery;
    trace_pair.eg.threats += (white_threats - black_threats) / 3;

    const int white_outposts = outpost_score(board, Color::White, pawn_attacks[static_cast<int>(Color::Black)]);
    const int black_outposts = outpost_score(board, Color::Black, pawn_attacks[static_cast<int>(Color::White)]);
    trace_pair.mg.outposts += white_outposts - black_outposts;
    trace_pair.eg.outposts += (white_outposts - black_outposts) / 2;

    const int white_rooks = rook_file_score(board, Color::White);
    const int black_rooks = rook_file_score(board, Color::Black);
    trace_pair.mg.rook_files += white_rooks - black_rooks;
    trace_pair.eg.rook_files += (white_rooks - black_rooks) / 2;

    const int white_center = center_control_score(board, Color::White, white_attacks);
    const int black_center = center_control_score(board, Color::Black, black_attacks);
    trace_pair.mg.center_control += white_center - black_center;
    trace_pair.eg.center_control += (white_center - black_center) / 3;

    const int white_space = space_score(board, Color::White, white_attacks, pawn_attacks[static_cast<int>(Color::Black)]);
    const int black_space = space_score(board, Color::Black, black_attacks, pawn_attacks[static_cast<int>(Color::White)]);
    trace_pair.mg.space += white_space - black_space;
    trace_pair.eg.space += (white_space - black_space) / 2;

    const int white_bishop_quality = bishop_quality_score(board, Color::White, white_attacks);
    const int black_bishop_quality = bishop_quality_score(board, Color::Black, black_attacks);
    trace_pair.mg.bishop_quality += white_bishop_quality - black_bishop_quality;
    trace_pair.eg.bishop_quality += (white_bishop_quality - black_bishop_quality) / 2;

    const int white_pawn_dynamics = pawn_dynamics_score(board, Color::White, pawn_attacks[static_cast<int>(Color::Black)]);
    const int black_pawn_dynamics = pawn_dynamics_score(board, Color::Black, pawn_attacks[static_cast<int>(Color::White)]);
    trace_pair.mg.pawn_dynamics += white_pawn_dynamics - black_pawn_dynamics;
    trace_pair.eg.pawn_dynamics += (white_pawn_dynamics - black_pawn_dynamics) / 2;

    trace_pair.mg.development += development_score(board, Color::White);
    trace_pair.mg.development -= development_score(board, Color::Black);

    const int material_balance = material_by_color[static_cast<int>(Color::White)]
        - material_by_color[static_cast<int>(Color::Black)];
    const int total_non_pawn = non_pawn_material(board, Color::White) + non_pawn_material(board, Color::Black);
    const int white_trade = trade_context_score(
        board,
        Color::White,
        material_balance,
        total_non_pawn,
        white_attack_on_black_king,
        black_attack_on_white_king,
        white_space,
        black_space
    );
    const int black_trade = trade_context_score(
        board,
        Color::Black,
        -material_balance,
        total_non_pawn,
        black_attack_on_white_king,
        white_attack_on_black_king,
        black_space,
        white_space
    );
    trace_pair.mg.trade_context += white_trade - black_trade;
    trace_pair.eg.trade_context += (white_trade - black_trade) / 2;

    phase = std::min(phase, kMaxPhase);
    return blend_trace(trace_pair, phase);
}

int evaluate_white_perspective(const Board& board) {
    return evaluate_trace_white_perspective(board).total;
}

int evaluate(const Board& board) {
    const int score = evaluate_white_perspective(board);
    return board.side_to_move() == Color::White ? score : -score;
}

}  // namespace chess::engine
