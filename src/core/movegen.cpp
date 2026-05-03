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

bool slider_attacks_along_line(PieceType slider, Square from, Square to) {
    const int file_delta = std::abs(file_of(from) - file_of(to));
    const int rank_delta = std::abs(rank_of(from) - rank_of(to));
    const bool diagonal = file_delta == rank_delta;
    const bool orthogonal = file_of(from) == file_of(to) || rank_of(from) == rank_of(to);
    switch (slider) {
        case PieceType::Bishop:
            return diagonal;
        case PieceType::Rook:
            return orthogonal;
        case PieceType::Queen:
            return diagonal || orthogonal;
        default:
            return false;
    }
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

bool contains_same_move_identity(const MoveList& moves, const Move& candidate) {
    return std::any_of(moves.begin(), moves.end(), [&](const Move& move) {
        return move.from == candidate.from
            && move.to == candidate.to
            && move.promotion == candidate.promotion;
    });
}

void push_unique(MoveList& moves, const Move& move) {
    if (!contains_same_move_identity(moves, move)) {
        moves.push_back(move);
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

void add_pawn_noisy_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    const int file = file_of(from);
    const int rank = rank_of(from);
    const int forward = color == Color::White ? 8 : -8;
    const int promotion_from_rank = color == Color::White ? 6 : 1;

    const Square one_forward = from + forward;
    if (rank == promotion_from_rank
        && is_valid_square(one_forward)
        && board.piece_at(one_forward) == Piece::None
        && (target_mask & square_bb(one_forward)) != 0) {
        add_promotion_moves(moves, from, one_forward, Quiet);
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

void add_pawn_quiet_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    const int rank = rank_of(from);
    const int forward = color == Color::White ? 8 : -8;
    const int start_rank = color == Color::White ? 1 : 6;
    const int promotion_from_rank = color == Color::White ? 6 : 1;

    const Square one_forward = from + forward;
    if (!is_valid_square(one_forward) || board.piece_at(one_forward) != Piece::None || rank == promotion_from_rank) {
        return;
    }

    if ((target_mask & square_bb(one_forward)) != 0) {
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

void add_piece_capture_target_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard attacks, Bitboard target_mask = kAllSquares) {
    const Bitboard targets = attacks
        & board.occupancy(opposite(color))
        & ~board.pieces(opposite(color), PieceType::King)
        & target_mask;
    add_piece_target_moves(board, moves, from, color, targets);
}

void add_piece_quiet_target_moves(const Board& board, MoveList& moves, Square from, Color, Bitboard attacks, Bitboard target_mask = kAllSquares) {
    const Bitboard targets = attacks & ~board.occupancy() & target_mask;
    for_each_square(targets, [&](Square to) {
        moves.push_back(Move{from, to, PieceType::None, Quiet});
    });
}

void add_knight_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    const Bitboard targets = attacks::knight_attacks(from)
        & ~board.occupancy(color)
        & ~board.pieces(opposite(color), PieceType::King)
        & target_mask;
    add_piece_target_moves(board, moves, from, color, targets);
}

void add_knight_noisy_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    add_piece_capture_target_moves(board, moves, from, color, attacks::knight_attacks(from), target_mask);
}

void add_knight_quiet_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    add_piece_quiet_target_moves(board, moves, from, color, attacks::knight_attacks(from), target_mask);
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

Bitboard slider_attacks_from(const Board& board, Square from, PieceType slider) {
    switch (slider) {
        case PieceType::Bishop:
            return attacks::bishop_attacks(from, board.occupancy());
        case PieceType::Rook:
            return attacks::rook_attacks(from, board.occupancy());
        case PieceType::Queen:
            return attacks::queen_attacks(from, board.occupancy());
        default:
            return 0;
    }
}

void add_slider_noisy_moves(
    const Board& board,
    MoveList& moves,
    Square from,
    Color color,
    PieceType slider,
    Bitboard target_mask = kAllSquares
) {
    add_piece_capture_target_moves(board, moves, from, color, slider_attacks_from(board, from, slider), target_mask);
}

void add_slider_quiet_moves(
    const Board& board,
    MoveList& moves,
    Square from,
    Color color,
    PieceType slider,
    Bitboard target_mask = kAllSquares
) {
    add_piece_quiet_target_moves(board, moves, from, color, slider_attacks_from(board, from, slider), target_mask);
}

void add_king_step_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    const Bitboard targets = attacks::king_attacks(from)
        & ~board.occupancy(color)
        & ~board.pieces(opposite(color), PieceType::King)
        & target_mask;
    add_piece_target_moves(board, moves, from, color, targets);
}

void add_king_noisy_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    add_piece_capture_target_moves(board, moves, from, color, attacks::king_attacks(from), target_mask);
}

void add_king_quiet_moves(const Board& board, MoveList& moves, Square from, Color color, Bitboard target_mask = kAllSquares) {
    add_piece_quiet_target_moves(board, moves, from, color, attacks::king_attacks(from), target_mask);
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

void add_quiet_castling_checks(const Board& board, MoveList& moves, Square from, Color color) {
    MoveList castling_moves;
    add_castling_moves(board, castling_moves, from, color);
    for (const Move& move : castling_moves) {
        if (move_gives_check(board, move)) {
            push_unique(moves, move);
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

void add_piece_noisy_moves(const Board& board, MoveList& moves, Square from, Color color, PieceType type, Bitboard target_mask = kAllSquares) {
    switch (type) {
        case PieceType::Pawn:
            add_pawn_noisy_moves(board, moves, from, color, target_mask);
            break;
        case PieceType::Knight:
            add_knight_noisy_moves(board, moves, from, color, target_mask);
            break;
        case PieceType::Bishop:
        case PieceType::Rook:
        case PieceType::Queen:
            add_slider_noisy_moves(board, moves, from, color, type, target_mask);
            break;
        case PieceType::King:
            add_king_noisy_moves(board, moves, from, color, target_mask);
            break;
        case PieceType::None:
            break;
    }
}

void add_piece_quiet_moves(const Board& board, MoveList& moves, Square from, Color color, PieceType type, Bitboard target_mask = kAllSquares) {
    switch (type) {
        case PieceType::Pawn:
            add_pawn_quiet_moves(board, moves, from, color, target_mask);
            break;
        case PieceType::Knight:
            add_knight_quiet_moves(board, moves, from, color, target_mask);
            break;
        case PieceType::Bishop:
        case PieceType::Rook:
        case PieceType::Queen:
            add_slider_quiet_moves(board, moves, from, color, type, target_mask);
            break;
        case PieceType::King:
            add_king_quiet_moves(board, moves, from, color, target_mask);
            break;
        case PieceType::None:
            break;
    }
}

Bitboard direct_quiet_check_targets(const Board& board, Square from, Color color, PieceType type, Square enemy_king) {
    Bitboard occupancy_after_departure = board.occupancy() & ~square_bb(from);
    switch (type) {
        case PieceType::Pawn: {
            Bitboard targets = 0;
            const int forward = color == Color::White ? 8 : -8;
            const int rank = rank_of(from);
            const int start_rank = color == Color::White ? 1 : 6;
            const int promotion_from_rank = color == Color::White ? 6 : 1;
            const Square one_forward = from + forward;
            if (rank != promotion_from_rank
                && is_valid_square(one_forward)
                && (attacks::pawn_attacks(color, one_forward) & square_bb(enemy_king)) != 0) {
                targets |= square_bb(one_forward);
            }
            const Square two_forward = from + 2 * forward;
            if (rank == start_rank
                && is_valid_square(two_forward)
                && (attacks::pawn_attacks(color, two_forward) & square_bb(enemy_king)) != 0) {
                targets |= square_bb(two_forward);
            }
            return targets;
        }
        case PieceType::Knight:
            return attacks::knight_attacks(enemy_king);
        case PieceType::Bishop:
            return attacks::bishop_attacks(enemy_king, occupancy_after_departure);
        case PieceType::Rook:
            return attacks::rook_attacks(enemy_king, occupancy_after_departure);
        case PieceType::Queen:
            return attacks::queen_attacks(enemy_king, occupancy_after_departure);
        case PieceType::King:
            return attacks::king_attacks(enemy_king);
        case PieceType::None:
            return 0;
    }
    return 0;
}

Bitboard discovered_check_blockers(const Board& board, Color color, Square enemy_king) {
    Bitboard blockers = 0;
    const Bitboard occupancy = board.occupancy();
    const Bitboard own_occupancy = board.occupancy(color);
    const Bitboard sliders = board.pieces(color, PieceType::Bishop)
        | board.pieces(color, PieceType::Rook)
        | board.pieces(color, PieceType::Queen);

    for_each_square(sliders, [&](Square slider_square) {
        const PieceType slider_type = type_of(board.piece_at(slider_square));
        if (!slider_attacks_along_line(slider_type, slider_square, enemy_king)) {
            return;
        }

        const Bitboard between = attacks::between(slider_square, enemy_king);
        const Bitboard occupied_between = between & occupancy;
        if (__builtin_popcountll(occupied_between) != 1) {
            return;
        }

        const Square blocker = __builtin_ctzll(occupied_between);
        if ((own_occupancy & square_bb(blocker)) != 0 && type_of(board.piece_at(blocker)) != PieceType::King) {
            blockers |= square_bb(blocker);
        }
    });

    return blockers;
}

}  // namespace

bool move_gives_check(const Board& board, const Move& move) {
    if (!is_valid_square(move.from) || !is_valid_square(move.to)) {
        return false;
    }

    const Piece moved = board.piece_at(move.from);
    if (moved == Piece::None) {
        return false;
    }

    const Color moving_side = board.side_to_move();
    const Color enemy = opposite(moving_side);
    const Square enemy_king = board.king_square(enemy);
    if (!is_valid_square(enemy_king)) {
        return false;
    }

    if (move.is_castling() || (move.flags & EnPassant) != 0) {
        Board copy = board;
        const UndoState undo = copy.make_move(move);
        const bool gives_check = copy.in_check(enemy);
        copy.unmake_move(undo);
        return gives_check;
    }

    const Bitboard from_bit = square_bb(move.from);
    const Bitboard to_bit = square_bb(move.to);
    Bitboard occupancy_after = board.occupancy();
    occupancy_after &= ~from_bit;
    occupancy_after |= to_bit;

    const PieceType moved_type = move.is_promotion() ? move.promotion : type_of(moved);
    if ((attacks::piece_attacks(moved_type, moving_side, move.to, occupancy_after) & square_bb(enemy_king)) != 0) {
        return true;
    }

    Bitboard bishops = board.pieces(moving_side, PieceType::Bishop);
    Bitboard rooks = board.pieces(moving_side, PieceType::Rook);
    Bitboard queens = board.pieces(moving_side, PieceType::Queen);

    switch (type_of(moved)) {
        case PieceType::Bishop:
            bishops &= ~from_bit;
            break;
        case PieceType::Rook:
            rooks &= ~from_bit;
            break;
        case PieceType::Queen:
            queens &= ~from_bit;
            break;
        default:
            break;
    }

    switch (moved_type) {
        case PieceType::Bishop:
            bishops |= to_bit;
            break;
        case PieceType::Rook:
            rooks |= to_bit;
            break;
        case PieceType::Queen:
            queens |= to_bit;
            break;
        default:
            break;
    }

    return (attacks::bishop_attacks(enemy_king, occupancy_after) & (bishops | queens)) != 0
        || (attacks::rook_attacks(enemy_king, occupancy_after) & (rooks | queens)) != 0;
}

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

MoveList generate_pseudo_legal_noisy_moves(const Board& board) {
    MoveList moves;
    moves.reserve(64);

    const Color color = board.side_to_move();
    for (Square from = 0; from < kBoardSquareCount; ++from) {
        const Piece piece = board.piece_at(from);
        if (piece == Piece::None || color_of(piece) != color) {
            continue;
        }

        add_piece_noisy_moves(board, moves, from, color, type_of(piece));
    }

    return moves;
}

MoveList generate_pseudo_legal_quiet_checks(const Board& board) {
    MoveList moves;
    moves.reserve(32);

    const Color color = board.side_to_move();
    const Square enemy_king = board.king_square(opposite(color));
    if (!is_valid_square(enemy_king)) {
        return moves;
    }

    const Bitboard discovery_blockers = discovered_check_blockers(board, color, enemy_king);
    for (Square from = 0; from < kBoardSquareCount; ++from) {
        const Piece piece = board.piece_at(from);
        if (piece == Piece::None || color_of(piece) != color) {
            continue;
        }

        const PieceType type = type_of(piece);
        const Bitboard direct_targets = direct_quiet_check_targets(board, from, color, type, enemy_king);
        MoveList direct_candidates;
        add_piece_quiet_moves(board, direct_candidates, from, color, type, direct_targets);
        for (const Move& move : direct_candidates) {
            push_unique(moves, move);
        }

        if ((discovery_blockers & square_bb(from)) != 0) {
            MoveList discovered_candidates;
            add_piece_quiet_moves(board, discovered_candidates, from, color, type);
            for (const Move& move : discovered_candidates) {
                if (move_gives_check(board, move)) {
                    push_unique(moves, move);
                }
            }
        }

        if (type == PieceType::King) {
            add_quiet_castling_checks(board, moves, from, color);
        }
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
