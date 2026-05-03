#include "chess/core/movegen.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

#include "chess/core/attacks.h"

namespace chess {

namespace {

constexpr Bitboard kAllSquares = ~Bitboard{0};

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

bool is_slider(PieceType type) {
    return type == PieceType::Bishop || type == PieceType::Rook || type == PieceType::Queen;
}

bool can_slider_block_check(Square king, Square checker, PieceType checker_type) {
    if (checker_type == PieceType::Bishop) {
        return attacks::between(king, checker) != 0;
    }
    if (checker_type == PieceType::Rook) {
        return attacks::between(king, checker) != 0;
    }
    if (checker_type == PieceType::Queen) {
        return attacks::between(king, checker) != 0;
    }
    return false;
}

template <typename Callback>
void for_each_square(Bitboard squares, Callback callback) {
    while (squares != 0) {
        const Square square = __builtin_ctzll(squares);
        squares &= squares - 1;
        callback(square);
    }
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

void add_pawn_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    const int file = file_of(from);
    const int rank = rank_of(from);
    const int forward = color == Color::White ? 8 : -8;
    const int start_rank = color == Color::White ? 1 : 6;
    const int promotion_from_rank = color == Color::White ? 6 : 1;

    const Square one_forward = from + forward;
    if (is_valid_square(one_forward) && board.piece_at(one_forward) == Piece::None) {
        const bool one_forward_allowed = (target_mask & square_bb(one_forward)) != 0;
        if (rank == promotion_from_rank && one_forward_allowed) {
            add_promotion_moves(moves, from, one_forward, Quiet);
        } else if (rank != promotion_from_rank) {
            if (one_forward_allowed) {
                moves.push_back(Move{from, one_forward, PieceType::None, Quiet});
            }
            const Square two_forward = from + 2 * forward;
            if (rank == start_rank
                && is_valid_square(two_forward)
                && board.piece_at(two_forward) == Piece::None
                && (target_mask & square_bb(two_forward)) != 0) {
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

        const bool target_allowed = (target_mask & square_bb(to)) != 0;
        if (target_allowed && has_enemy_piece(board, to, color) && !is_enemy_king(board, to, color)) {
            if (rank == promotion_from_rank) {
                add_promotion_moves(moves, from, to, Capture);
            } else {
                moves.push_back(Move{from, to, PieceType::None, Capture});
            }
        } else if (to == board.en_passant_square()) {
            const Square captured_square = color == Color::White ? to - 8 : to + 8;
            if (target_allowed || (is_valid_square(captured_square) && (target_mask & square_bb(captured_square)) != 0)) {
                moves.push_back(Move{from, to, PieceType::None, static_cast<std::uint8_t>(Capture | EnPassant)});
            }
        }
    }
}

void add_piece_target_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard targets) {
    const Bitboard enemy_occupancy = board.occupancy(opposite(color));
    for_each_square(targets, [&](Square to) {
        moves.push_back(Move{
            from,
            to,
            PieceType::None,
            static_cast<std::uint8_t>((enemy_occupancy & square_bb(to)) != 0 ? Capture : Quiet),
        });
    });
}

void add_knight_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    const Bitboard targets = attacks::knight_attacks(from)
        & ~board.occupancy(color)
        & ~board.pieces(opposite(color), PieceType::King)
        & target_mask;
    add_piece_target_moves(board, moves, from, color, targets);
}

void add_slider_moves(
    const Board& board,
    MoveList& moves,
    Square from,
    Color color,
    PieceType slider,
    Bitboard target_mask = kAllSquares
) {
    Bitboard attacks = 0;
    switch (slider) {
        case PieceType::Bishop:
            attacks = attacks::bishop_attacks(from, board.occupancy());
            break;
        case PieceType::Rook:
            attacks = attacks::rook_attacks(from, board.occupancy());
            break;
        case PieceType::Queen:
            attacks = attacks::queen_attacks(from, board.occupancy());
            break;
        default:
            return;
    }

    const Bitboard targets = attacks
        & ~board.occupancy(color)
        & ~board.pieces(opposite(color), PieceType::King)
        & target_mask;
    add_piece_target_moves(board, moves, from, color, targets);
}

void add_king_step_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    const Bitboard targets = attacks::king_attacks(from)
        & ~board.occupancy(color)
        & ~board.pieces(opposite(color), PieceType::King)
        & target_mask;
    add_piece_target_moves(board, moves, from, color, targets);
}

void add_castling_moves(const Board& board, MoveList& moves, Square from, Color color) {
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

void add_king_moves(const Board& board, MoveList& moves, Square from, Color color) {
    add_king_step_moves(board, moves, from, color);
    add_castling_moves(board, moves, from, color);
}

void add_piece_moves(const Board& board, MoveList& moves, Square from, Color color, PieceType type, Bitboard target_mask = kAllSquares) {
    switch (type) {
        case PieceType::Pawn:
            add_pawn_moves(board, moves, from, color, target_mask);
            break;
        case PieceType::Knight:
            add_knight_moves(board, moves, from, color, target_mask);
            break;
        case PieceType::Bishop:
        case PieceType::Rook:
        case PieceType::Queen:
            add_slider_moves(board, moves, from, color, type, target_mask);
            break;
        case PieceType::King:
            if (target_mask == kAllSquares) {
                add_king_moves(board, moves, from, color);
            } else {
                add_king_step_moves(board, moves, from, color, target_mask);
            }
            break;
        case PieceType::None:
            break;
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

        add_piece_moves(board, moves, from, color, type_of(piece));
    }

    return moves;
}

MoveList generate_pseudo_legal_check_evasions(const Board& board) {
    const Color color = board.side_to_move();
    const Color enemy = opposite(color);
    const Square king = board.king_square(color);
    if (!is_valid_square(king) || !board.in_check(color)) {
        return generate_pseudo_legal_moves(board);
    }

    MoveList moves;
    moves.reserve(32);
    add_king_step_moves(board, moves, king, color);

    const Bitboard checkers = attacks::attackers_to(board, king, enemy);
    if (__builtin_popcountll(checkers) != 1) {
        return moves;
    }

    const Square checker = __builtin_ctzll(checkers);
    const Piece checker_piece = board.piece_at(checker);
    Bitboard evasion_targets = square_bb(checker);
    if (checker_piece != Piece::None && is_slider(type_of(checker_piece)) && can_slider_block_check(king, checker, type_of(checker_piece))) {
        evasion_targets |= attacks::between(king, checker);
    }

    for (Square from = 0; from < kBoardSquareCount; ++from) {
        if (from == king) {
            continue;
        }
        const Piece piece = board.piece_at(from);
        if (piece == Piece::None || color_of(piece) != color) {
            continue;
        }

        add_piece_moves(board, moves, from, color, type_of(piece), evasion_targets);
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
