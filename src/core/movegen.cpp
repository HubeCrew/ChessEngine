#include "chess/core/movegen.h"

#include <algorithm>
#include <stdexcept>

namespace chess {

namespace {

bool has_own_piece(const Board& board, Square square, Color color) {
    return is_valid_square(square)
        && board.piece_at(square) != Piece::None
        && color_of(board.piece_at(square)) == color;
}

bool has_enemy_piece(const Board& board, Square square, Color color) {
    return is_valid_square(square)
        && board.piece_at(square) != Piece::None
        && color_of(board.piece_at(square)) != color;
}

bool is_enemy_king(const Board& board, Square square, Color color) {
    return is_valid_square(square)
        && board.piece_at(square) != Piece::None
        && color_of(board.piece_at(square)) != color
        && type_of(board.piece_at(square)) == PieceType::King;
}

void add_promotion_moves(MoveList& moves, Square from, Square to, std::uint8_t flags) {
    for (const PieceType promotion : {
             PieceType::Queen,
             PieceType::Rook,
             PieceType::Bishop,
             PieceType::Knight,
         }) {
        moves.push_back(Move{from, to, promotion, static_cast<std::uint8_t>(flags | Promotion)});
    }
}

void add_pawn_moves(const Board& board, MoveList& moves, Square from, Color color) {
    const int file = file_of(from);
    const int rank = rank_of(from);
    const int forward = color == Color::White ? 8 : -8;
    const int start_rank = color == Color::White ? 1 : 6;
    const int promotion_from_rank = color == Color::White ? 6 : 1;

    const Square one_forward = from + forward;
    if (is_valid_square(one_forward) && board.piece_at(one_forward) == Piece::None) {
        if (rank == promotion_from_rank) {
            add_promotion_moves(moves, from, one_forward, Quiet);
        } else {
            moves.push_back(Move{from, one_forward, PieceType::None, Quiet});
            const Square two_forward = from + 2 * forward;
            if (rank == start_rank && board.piece_at(two_forward) == Piece::None) {
                moves.push_back(Move{from, two_forward, PieceType::None, DoublePawnPush});
            }
        }
    }

    const std::array<int, 2> capture_offsets = color == Color::White
        ? std::array<int, 2>{7, 9}
        : std::array<int, 2>{-9, -7};

    for (const int offset : capture_offsets) {
        if ((offset == 7 || offset == -9) && file == 0) {
            continue;
        }
        if ((offset == 9 || offset == -7) && file == 7) {
            continue;
        }

        const Square to = from + offset;
        if (!is_valid_square(to)) {
            continue;
        }

        if (has_enemy_piece(board, to, color) && !is_enemy_king(board, to, color)) {
            if (rank == promotion_from_rank) {
                add_promotion_moves(moves, from, to, Capture);
            } else {
                moves.push_back(Move{from, to, PieceType::None, Capture});
            }
        } else if (to == board.en_passant_square()) {
            moves.push_back(Move{from, to, PieceType::None, static_cast<std::uint8_t>(Capture | EnPassant)});
        }
    }
}

void add_knight_moves(const Board& board, MoveList& moves, Square from, Color color) {
    constexpr std::array<std::pair<int, int>, 8> offsets{{
        {1, 2}, {2, 1}, {2, -1}, {1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2},
    }};

    for (const auto& [df, dr] : offsets) {
        const int file = file_of(from) + df;
        const int rank = rank_of(from) + dr;
        if (file < 0 || file >= 8 || rank < 0 || rank >= 8) {
            continue;
        }
        const Square to = make_square(file, rank);
        if (has_own_piece(board, to, color) || is_enemy_king(board, to, color)) {
            continue;
        }
        moves.push_back(Move{
            from,
            to,
            PieceType::None,
            static_cast<std::uint8_t>(has_enemy_piece(board, to, color) ? Capture : Quiet),
        });
    }
}

void add_slider_moves(
    const Board& board,
    MoveList& moves,
    Square from,
    Color color,
    std::initializer_list<std::pair<int, int>> directions
) {
    for (const auto& [df, dr] : directions) {
        int file = file_of(from) + df;
        int rank = rank_of(from) + dr;
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const Square to = make_square(file, rank);
            if (has_own_piece(board, to, color)) {
                break;
            }
            if (is_enemy_king(board, to, color)) {
                break;
            }
            moves.push_back(Move{
                from,
                to,
                PieceType::None,
                static_cast<std::uint8_t>(has_enemy_piece(board, to, color) ? Capture : Quiet),
            });
            if (has_enemy_piece(board, to, color)) {
                break;
            }
            file += df;
            rank += dr;
        }
    }
}

void add_king_moves(const Board& board, MoveList& moves, Square from, Color color) {
    constexpr std::array<std::pair<int, int>, 8> offsets{{
        {1, 1}, {1, 0}, {1, -1}, {0, -1},
        {-1, -1}, {-1, 0}, {-1, 1}, {0, 1},
    }};

    for (const auto& [df, dr] : offsets) {
        const int file = file_of(from) + df;
        const int rank = rank_of(from) + dr;
        if (file < 0 || file >= 8 || rank < 0 || rank >= 8) {
            continue;
        }
        const Square to = make_square(file, rank);
        if (has_own_piece(board, to, color) || is_enemy_king(board, to, color)) {
            continue;
        }
        moves.push_back(Move{
            from,
            to,
            PieceType::None,
            static_cast<std::uint8_t>(has_enemy_piece(board, to, color) ? Capture : Quiet),
        });
    }

    const Color enemy = opposite(color);
    if (board.in_check(color)) {
        return;
    }

    if (color == Color::White) {
        if ((board.castling_rights() & WhiteKingSide) != 0
            && board.piece_at(make_square(7, 0)) == Piece::WhiteRook
            && board.piece_at(make_square(5, 0)) == Piece::None
            && board.piece_at(make_square(6, 0)) == Piece::None
            && !board.is_square_attacked(make_square(5, 0), enemy)
            && !board.is_square_attacked(make_square(6, 0), enemy)) {
            moves.push_back(Move{from, make_square(6, 0), PieceType::None, KingCastle});
        }
        if ((board.castling_rights() & WhiteQueenSide) != 0
            && board.piece_at(make_square(0, 0)) == Piece::WhiteRook
            && board.piece_at(make_square(1, 0)) == Piece::None
            && board.piece_at(make_square(2, 0)) == Piece::None
            && board.piece_at(make_square(3, 0)) == Piece::None
            && !board.is_square_attacked(make_square(2, 0), enemy)
            && !board.is_square_attacked(make_square(3, 0), enemy)) {
            moves.push_back(Move{from, make_square(2, 0), PieceType::None, QueenCastle});
        }
    } else {
        if ((board.castling_rights() & BlackKingSide) != 0
            && board.piece_at(make_square(7, 7)) == Piece::BlackRook
            && board.piece_at(make_square(5, 7)) == Piece::None
            && board.piece_at(make_square(6, 7)) == Piece::None
            && !board.is_square_attacked(make_square(5, 7), enemy)
            && !board.is_square_attacked(make_square(6, 7), enemy)) {
            moves.push_back(Move{from, make_square(6, 7), PieceType::None, KingCastle});
        }
        if ((board.castling_rights() & BlackQueenSide) != 0
            && board.piece_at(make_square(0, 7)) == Piece::BlackRook
            && board.piece_at(make_square(1, 7)) == Piece::None
            && board.piece_at(make_square(2, 7)) == Piece::None
            && board.piece_at(make_square(3, 7)) == Piece::None
            && !board.is_square_attacked(make_square(2, 7), enemy)
            && !board.is_square_attacked(make_square(3, 7), enemy)) {
            moves.push_back(Move{from, make_square(2, 7), PieceType::None, QueenCastle});
        }
    }
}

}  // namespace

MoveList generate_pseudo_legal_moves(const Board& board) {
    MoveList moves;
    moves.reserve(128);

    const Color color = board.side_to_move();
    for (Square from = 0; from < kBoardSquareCount; ++from) {
        const Piece piece = board.piece_at(from);
        if (piece == Piece::None || color_of(piece) != color) {
            continue;
        }

        switch (type_of(piece)) {
            case PieceType::Pawn:
                add_pawn_moves(board, moves, from, color);
                break;
            case PieceType::Knight:
                add_knight_moves(board, moves, from, color);
                break;
            case PieceType::Bishop:
                add_slider_moves(board, moves, from, color, {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}});
                break;
            case PieceType::Rook:
                add_slider_moves(board, moves, from, color, {{1, 0}, {0, -1}, {-1, 0}, {0, 1}});
                break;
            case PieceType::Queen:
                add_slider_moves(
                    board,
                    moves,
                    from,
                    color,
                    {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}, {1, 0}, {0, -1}, {-1, 0}, {0, 1}}
                );
                break;
            case PieceType::King:
                add_king_moves(board, moves, from, color);
                break;
            case PieceType::None:
                break;
        }
    }

    return moves;
}

MoveList generate_legal_moves(Board& board) {
    const Color moving_side = board.side_to_move();
    MoveList legal_moves;
    legal_moves.reserve(64);

    for (const Move& move : generate_pseudo_legal_moves(board)) {
        const UndoState undo = board.make_move(move);
        const bool leaves_king_safe = !board.in_check(moving_side);
        board.unmake_move(undo);
        if (leaves_king_safe) {
            legal_moves.push_back(move);
        }
    }

    return legal_moves;
}

bool is_legal_move(Board& board, const Move& move) {
    const MoveList legal_moves = generate_legal_moves(board);
    return std::find(legal_moves.begin(), legal_moves.end(), move) != legal_moves.end();
}

Move parse_uci_move(Board& board, std::string_view text) {
    if (text.size() < 4 || text.size() > 5) {
        throw std::invalid_argument("UCI move must contain four or five characters");
    }

    const Square from = square_from_string(text.substr(0, 2));
    const Square to = square_from_string(text.substr(2, 2));
    const PieceType promotion = text.size() == 5 ? promotion_from_uci(text[4]) : PieceType::None;
    if (!is_valid_square(from) || !is_valid_square(to) || (text.size() == 5 && promotion == PieceType::None)) {
        throw std::invalid_argument("UCI move has invalid square or promotion");
    }

    const MoveList legal_moves = generate_legal_moves(board);
    const auto found = std::find_if(legal_moves.begin(), legal_moves.end(), [&](const Move& move) {
        return move.from == from && move.to == to && move.promotion == promotion;
    });
    if (found == legal_moves.end()) {
        throw std::invalid_argument("UCI move is not legal in the current position");
    }
    return *found;
}

}  // namespace chess
