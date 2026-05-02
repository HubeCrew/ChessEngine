#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace chess {

using Bitboard = std::uint64_t;
using Square = int;

constexpr Square kNoSquare = -1;
constexpr int kBoardSquareCount = 64;

enum class Color : std::uint8_t {
    White = 0,
    Black = 1,
};

constexpr Color opposite(Color color) {
    return color == Color::White ? Color::Black : Color::White;
}

enum class PieceType : std::uint8_t {
    Pawn = 0,
    Knight = 1,
    Bishop = 2,
    Rook = 3,
    Queen = 4,
    King = 5,
    None = 6,
};

enum class Piece : std::uint8_t {
    None = 0,
    WhitePawn,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,
    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing,
};

enum CastlingRights : std::uint8_t {
    WhiteKingSide = 1 << 0,
    WhiteQueenSide = 1 << 1,
    BlackKingSide = 1 << 2,
    BlackQueenSide = 1 << 3,
};

constexpr int file_of(Square square) {
    return square & 7;
}

constexpr int rank_of(Square square) {
    return square >> 3;
}

constexpr bool is_valid_square(Square square) {
    return square >= 0 && square < kBoardSquareCount;
}

constexpr Bitboard square_bb(Square square) {
    return Bitboard{1} << square;
}

constexpr Square make_square(int file, int rank) {
    return rank * 8 + file;
}

constexpr bool is_white_piece(Piece piece) {
    return piece >= Piece::WhitePawn && piece <= Piece::WhiteKing;
}

constexpr bool is_black_piece(Piece piece) {
    return piece >= Piece::BlackPawn && piece <= Piece::BlackKing;
}

constexpr bool is_piece(Piece piece) {
    return piece != Piece::None;
}

constexpr Color color_of(Piece piece) {
    return is_black_piece(piece) ? Color::Black : Color::White;
}

constexpr PieceType type_of(Piece piece) {
    switch (piece) {
        case Piece::WhitePawn:
        case Piece::BlackPawn:
            return PieceType::Pawn;
        case Piece::WhiteKnight:
        case Piece::BlackKnight:
            return PieceType::Knight;
        case Piece::WhiteBishop:
        case Piece::BlackBishop:
            return PieceType::Bishop;
        case Piece::WhiteRook:
        case Piece::BlackRook:
            return PieceType::Rook;
        case Piece::WhiteQueen:
        case Piece::BlackQueen:
            return PieceType::Queen;
        case Piece::WhiteKing:
        case Piece::BlackKing:
            return PieceType::King;
        case Piece::None:
            return PieceType::None;
    }
    return PieceType::None;
}

constexpr Piece make_piece(Color color, PieceType type) {
    if (type == PieceType::None) {
        return Piece::None;
    }
    const int base = color == Color::White
        ? static_cast<int>(Piece::WhitePawn)
        : static_cast<int>(Piece::BlackPawn);
    return static_cast<Piece>(base + static_cast<int>(type));
}

char piece_to_char(Piece piece);
Piece char_to_piece(char value);
std::string square_to_string(Square square);
Square square_from_string(std::string_view value);

}  // namespace chess

