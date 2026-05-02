#include "chess/core/move.h"

#include <cctype>
#include <stdexcept>

namespace chess {

char piece_to_char(Piece piece) {
    switch (piece) {
        case Piece::WhitePawn:
            return 'P';
        case Piece::WhiteKnight:
            return 'N';
        case Piece::WhiteBishop:
            return 'B';
        case Piece::WhiteRook:
            return 'R';
        case Piece::WhiteQueen:
            return 'Q';
        case Piece::WhiteKing:
            return 'K';
        case Piece::BlackPawn:
            return 'p';
        case Piece::BlackKnight:
            return 'n';
        case Piece::BlackBishop:
            return 'b';
        case Piece::BlackRook:
            return 'r';
        case Piece::BlackQueen:
            return 'q';
        case Piece::BlackKing:
            return 'k';
        case Piece::None:
            return '.';
    }
    return '.';
}

Piece char_to_piece(char value) {
    switch (value) {
        case 'P':
            return Piece::WhitePawn;
        case 'N':
            return Piece::WhiteKnight;
        case 'B':
            return Piece::WhiteBishop;
        case 'R':
            return Piece::WhiteRook;
        case 'Q':
            return Piece::WhiteQueen;
        case 'K':
            return Piece::WhiteKing;
        case 'p':
            return Piece::BlackPawn;
        case 'n':
            return Piece::BlackKnight;
        case 'b':
            return Piece::BlackBishop;
        case 'r':
            return Piece::BlackRook;
        case 'q':
            return Piece::BlackQueen;
        case 'k':
            return Piece::BlackKing;
        default:
            return Piece::None;
    }
}

std::string square_to_string(Square square) {
    if (!is_valid_square(square)) {
        return "-";
    }
    std::string result;
    result.push_back(static_cast<char>('a' + file_of(square)));
    result.push_back(static_cast<char>('1' + rank_of(square)));
    return result;
}

Square square_from_string(std::string_view value) {
    if (value.size() != 2) {
        return kNoSquare;
    }
    const char file = value[0];
    const char rank = value[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        return kNoSquare;
    }
    return make_square(file - 'a', rank - '1');
}

PieceType promotion_from_uci(char value) {
    switch (static_cast<char>(std::tolower(static_cast<unsigned char>(value)))) {
        case 'n':
            return PieceType::Knight;
        case 'b':
            return PieceType::Bishop;
        case 'r':
            return PieceType::Rook;
        case 'q':
            return PieceType::Queen;
        default:
            return PieceType::None;
    }
}

char promotion_to_uci(PieceType type) {
    switch (type) {
        case PieceType::Knight:
            return 'n';
        case PieceType::Bishop:
            return 'b';
        case PieceType::Rook:
            return 'r';
        case PieceType::Queen:
            return 'q';
        case PieceType::Pawn:
        case PieceType::King:
        case PieceType::None:
            return '\0';
    }
    return '\0';
}

std::string move_to_uci(const Move& move) {
    if (!is_valid_square(move.from) || !is_valid_square(move.to)) {
        return "0000";
    }

    std::string result = square_to_string(move.from) + square_to_string(move.to);
    if (move.is_promotion()) {
        result.push_back(promotion_to_uci(move.promotion));
    }
    return result;
}

}  // namespace chess

