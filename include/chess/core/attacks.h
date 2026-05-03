#pragma once

#include <array>
#include <cassert>
#include <cstdlib>
#include <utility>

#include "chess/core/types.h"

namespace chess {

class Board;

namespace attacks {

namespace detail {

using Direction = std::pair<int, int>;

inline constexpr std::array<Direction, 4> kBishopDirections{{
    {1, 1}, {1, -1}, {-1, -1}, {-1, 1},
}};

inline constexpr std::array<Direction, 4> kRookDirections{{
    {1, 0}, {0, -1}, {-1, 0}, {0, 1},
}};

inline constexpr std::array<Direction, 8> kQueenDirections{{
    {1, 1}, {1, -1}, {-1, -1}, {-1, 1},
    {1, 0}, {0, -1}, {-1, 0}, {0, 1},
}};

constexpr Bitboard attacks_from_offsets(Square square, const std::array<Direction, 8>& offsets) {
    Bitboard attacks = 0;
    const int file = file_of(square);
    const int rank = rank_of(square);
    for (const auto& [df, dr] : offsets) {
        const int to_file = file + df;
        const int to_rank = rank + dr;
        if (to_file >= 0 && to_file < 8 && to_rank >= 0 && to_rank < 8) {
            attacks |= square_bb(make_square(to_file, to_rank));
        }
    }
    return attacks;
}

constexpr Bitboard pawn_attacks_from(Color color, Square square) {
    Bitboard attacks = 0;
    const int file = file_of(square);
    if (color == Color::White) {
        if (file > 0 && is_valid_square(square + 7)) {
            attacks |= square_bb(square + 7);
        }
        if (file < 7 && is_valid_square(square + 9)) {
            attacks |= square_bb(square + 9);
        }
    } else {
        if (file > 0 && is_valid_square(square - 9)) {
            attacks |= square_bb(square - 9);
        }
        if (file < 7 && is_valid_square(square - 7)) {
            attacks |= square_bb(square - 7);
        }
    }
    return attacks;
}

constexpr std::array<Bitboard, kBoardSquareCount> build_pawn_table(Color color) {
    std::array<Bitboard, kBoardSquareCount> table{};
    for (Square square = 0; square < kBoardSquareCount; ++square) {
        table[square] = pawn_attacks_from(color, square);
    }
    return table;
}

constexpr std::array<Bitboard, kBoardSquareCount> build_knight_table() {
    constexpr std::array<Direction, 8> offsets{{
        {1, 2}, {2, 1}, {2, -1}, {1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2},
    }};

    std::array<Bitboard, kBoardSquareCount> table{};
    for (Square square = 0; square < kBoardSquareCount; ++square) {
        table[square] = attacks_from_offsets(square, offsets);
    }
    return table;
}

constexpr std::array<Bitboard, kBoardSquareCount> build_king_table() {
    constexpr std::array<Direction, 8> offsets{{
        {1, 1}, {1, 0}, {1, -1}, {0, -1},
        {-1, -1}, {-1, 0}, {-1, 1}, {0, 1},
    }};

    std::array<Bitboard, kBoardSquareCount> table{};
    for (Square square = 0; square < kBoardSquareCount; ++square) {
        table[square] = attacks_from_offsets(square, offsets);
    }
    return table;
}

inline constexpr std::array<Bitboard, kBoardSquareCount> kWhitePawnAttacks = build_pawn_table(Color::White);
inline constexpr std::array<Bitboard, kBoardSquareCount> kBlackPawnAttacks = build_pawn_table(Color::Black);
inline constexpr std::array<Bitboard, kBoardSquareCount> kKnightAttacks = build_knight_table();
inline constexpr std::array<Bitboard, kBoardSquareCount> kKingAttacks = build_king_table();

template <std::size_t Size>
Bitboard slider_attacks(Square square, Bitboard occupancy, const std::array<Direction, Size>& directions) {
    assert(is_valid_square(square));
    Bitboard attacks = 0;
    const int file = file_of(square);
    const int rank = rank_of(square);

    for (const auto& [df, dr] : directions) {
        int to_file = file + df;
        int to_rank = rank + dr;
        while (to_file >= 0 && to_file < 8 && to_rank >= 0 && to_rank < 8) {
            const Square to = make_square(to_file, to_rank);
            attacks |= square_bb(to);
            if ((occupancy & square_bb(to)) != 0) {
                break;
            }
            to_file += df;
            to_rank += dr;
        }
    }

    return attacks;
}

inline bool same_diagonal(Square lhs, Square rhs) {
    return std::abs(file_of(lhs) - file_of(rhs)) == std::abs(rank_of(lhs) - rank_of(rhs));
}

inline bool same_line(Square lhs, Square rhs) {
    return file_of(lhs) == file_of(rhs) || rank_of(lhs) == rank_of(rhs);
}

inline int ray_step(Square from, Square to) {
    if (file_of(from) == file_of(to)) {
        return rank_of(to) > rank_of(from) ? 8 : -8;
    }
    if (rank_of(from) == rank_of(to)) {
        return file_of(to) > file_of(from) ? 1 : -1;
    }
    if (!same_diagonal(from, to)) {
        return 0;
    }

    const bool file_increases = file_of(to) > file_of(from);
    const bool rank_increases = rank_of(to) > rank_of(from);
    if (file_increases && rank_increases) {
        return 9;
    }
    if (!file_increases && rank_increases) {
        return 7;
    }
    if (file_increases) {
        return -7;
    }
    return -9;
}

}  // namespace detail

[[nodiscard]] inline Bitboard pawn_attacks(Color color, Square square) {
    assert(is_valid_square(square));
    return color == Color::White ? detail::kWhitePawnAttacks[square] : detail::kBlackPawnAttacks[square];
}

[[nodiscard]] inline Bitboard knight_attacks(Square square) {
    assert(is_valid_square(square));
    return detail::kKnightAttacks[square];
}

[[nodiscard]] inline Bitboard king_attacks(Square square) {
    assert(is_valid_square(square));
    return detail::kKingAttacks[square];
}

[[nodiscard]] inline Bitboard bishop_attacks(Square square, Bitboard occupancy) {
    return detail::slider_attacks(square, occupancy, detail::kBishopDirections);
}

[[nodiscard]] inline Bitboard rook_attacks(Square square, Bitboard occupancy) {
    return detail::slider_attacks(square, occupancy, detail::kRookDirections);
}

[[nodiscard]] inline Bitboard queen_attacks(Square square, Bitboard occupancy) {
    return detail::slider_attacks(square, occupancy, detail::kQueenDirections);
}

[[nodiscard]] inline Bitboard piece_attacks(PieceType type, Color color, Square square, Bitboard occupancy) {
    switch (type) {
        case PieceType::Pawn:
            return pawn_attacks(color, square);
        case PieceType::Knight:
            return knight_attacks(square);
        case PieceType::Bishop:
            return bishop_attacks(square, occupancy);
        case PieceType::Rook:
            return rook_attacks(square, occupancy);
        case PieceType::Queen:
            return queen_attacks(square, occupancy);
        case PieceType::King:
            return king_attacks(square);
        case PieceType::None:
            return 0;
    }
    return 0;
}

[[nodiscard]] inline Bitboard between(Square from, Square to) {
    assert(is_valid_square(from));
    assert(is_valid_square(to));
    if (from == to || (!detail::same_line(from, to) && !detail::same_diagonal(from, to))) {
        return 0;
    }

    const int step = detail::ray_step(from, to);
    if (step == 0) {
        return 0;
    }

    Bitboard mask = 0;
    for (Square square = from + step; square != to; square += step) {
        mask |= square_bb(square);
    }
    return mask;
}

[[nodiscard]] Bitboard attackers_to(const Board& board, Square square, Color by_color);
[[nodiscard]] bool is_square_attacked(const Board& board, Square square, Color by_color);

}  // namespace attacks

}  // namespace chess
