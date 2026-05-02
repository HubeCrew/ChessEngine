#include "chess/engine/static_exchange.h"

#include <algorithm>
#include <array>
#include <utility>

#include "chess/engine/evaluation.h"

namespace chess::engine {

namespace {

constexpr int color_index(Color color) {
    return static_cast<int>(color);
}

constexpr int type_index(PieceType type) {
    return static_cast<int>(type);
}

using PieceBoards = std::array<std::array<Bitboard, 6>, 2>;

struct Attacker {
    Square square = kNoSquare;
    PieceType type = PieceType::None;
};

Piece captured_piece_for(const Board& board, const Move& move) {
    if ((move.flags & EnPassant) != 0) {
        return make_piece(opposite(board.side_to_move()), PieceType::Pawn);
    }
    return board.piece_at(move.to);
}

Square captured_square_for(const Board& board, const Move& move) {
    if ((move.flags & EnPassant) == 0) {
        return move.to;
    }
    return board.side_to_move() == Color::White ? move.to - 8 : move.to + 8;
}

int promotion_gain(const Board& board, const Move& move) {
    if (!move.is_promotion()) {
        return 0;
    }

    const Piece moving_piece = board.piece_at(move.from);
    if (moving_piece == Piece::None || type_of(moving_piece) != PieceType::Pawn) {
        return 0;
    }

    return material_value(move.promotion) - material_value(PieceType::Pawn);
}

void remove_piece(PieceBoards& pieces, Color color, PieceType type, Square square) {
    if (type != PieceType::None && is_valid_square(square)) {
        pieces[color_index(color)][type_index(type)] &= ~square_bb(square);
    }
}

void add_piece(PieceBoards& pieces, Color color, PieceType type, Square square) {
    if (type != PieceType::None && is_valid_square(square)) {
        pieces[color_index(color)][type_index(type)] |= square_bb(square);
    }
}

Piece piece_on(const PieceBoards& pieces, Square square) {
    const Bitboard bit = square_bb(square);
    for (const Color color : {Color::White, Color::Black}) {
        for (const PieceType type : {
                 PieceType::Pawn,
                 PieceType::Knight,
                 PieceType::Bishop,
                 PieceType::Rook,
                 PieceType::Queen,
                 PieceType::King,
             }) {
            if ((pieces[color_index(color)][type_index(type)] & bit) != 0) {
                return make_piece(color, type);
            }
        }
    }
    return Piece::None;
}

Bitboard knight_attackers_to(const PieceBoards& pieces, Square target, Color color) {
    constexpr std::array<std::pair<int, int>, 8> offsets{{
        {1, 2}, {2, 1}, {2, -1}, {1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2},
    }};

    Bitboard attackers = 0;
    for (const auto& [df, dr] : offsets) {
        const int file = file_of(target) + df;
        const int rank = rank_of(target) + dr;
        if (file < 0 || file >= 8 || rank < 0 || rank >= 8) {
            continue;
        }
        const Square square = make_square(file, rank);
        attackers |= square_bb(square);
    }
    return attackers & pieces[color_index(color)][type_index(PieceType::Knight)];
}

Bitboard king_attackers_to(const PieceBoards& pieces, Square target, Color color) {
    constexpr std::array<std::pair<int, int>, 8> offsets{{
        {1, 1}, {1, 0}, {1, -1}, {0, -1},
        {-1, -1}, {-1, 0}, {-1, 1}, {0, 1},
    }};

    Bitboard attackers = 0;
    for (const auto& [df, dr] : offsets) {
        const int file = file_of(target) + df;
        const int rank = rank_of(target) + dr;
        if (file < 0 || file >= 8 || rank < 0 || rank >= 8) {
            continue;
        }
        const Square square = make_square(file, rank);
        attackers |= square_bb(square);
    }
    return attackers & pieces[color_index(color)][type_index(PieceType::King)];
}

Bitboard pawn_attackers_to(const PieceBoards& pieces, Square target, Color color) {
    const int file = file_of(target);
    Bitboard attackers = 0;

    if (color == Color::White) {
        if (file > 0 && is_valid_square(target - 9)) {
            attackers |= square_bb(target - 9);
        }
        if (file < 7 && is_valid_square(target - 7)) {
            attackers |= square_bb(target - 7);
        }
    } else {
        if (file > 0 && is_valid_square(target + 7)) {
            attackers |= square_bb(target + 7);
        }
        if (file < 7 && is_valid_square(target + 9)) {
            attackers |= square_bb(target + 9);
        }
    }

    return attackers & pieces[color_index(color)][type_index(PieceType::Pawn)];
}

Bitboard slider_attackers_to(
    const PieceBoards& pieces,
    Bitboard occupancy,
    Square target,
    Color color,
    const std::array<std::pair<int, int>, 4>& directions,
    PieceType primary_slider
) {
    Bitboard attackers = 0;
    for (const auto& [df, dr] : directions) {
        int file = file_of(target) + df;
        int rank = rank_of(target) + dr;
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const Square square = make_square(file, rank);
            if ((occupancy & square_bb(square)) == 0) {
                file += df;
                rank += dr;
                continue;
            }

            const Piece piece = piece_on(pieces, square);
            if (piece != Piece::None && color_of(piece) == color
                && (type_of(piece) == primary_slider || type_of(piece) == PieceType::Queen)) {
                attackers |= square_bb(square);
            }
            break;
        }
    }
    return attackers;
}

Bitboard attackers_to(const PieceBoards& pieces, Bitboard occupancy, Square target, Color color) {
    constexpr std::array<std::pair<int, int>, 4> bishop_dirs{{
        {1, 1}, {1, -1}, {-1, -1}, {-1, 1},
    }};
    constexpr std::array<std::pair<int, int>, 4> rook_dirs{{
        {1, 0}, {0, -1}, {-1, 0}, {0, 1},
    }};

    return pawn_attackers_to(pieces, target, color)
        | knight_attackers_to(pieces, target, color)
        | king_attackers_to(pieces, target, color)
        | slider_attackers_to(pieces, occupancy, target, color, bishop_dirs, PieceType::Bishop)
        | slider_attackers_to(pieces, occupancy, target, color, rook_dirs, PieceType::Rook);
}

Attacker least_valuable_attacker(const PieceBoards& pieces, Bitboard attackers, Color color) {
    for (const PieceType type : {
             PieceType::Pawn,
             PieceType::Knight,
             PieceType::Bishop,
             PieceType::Rook,
             PieceType::Queen,
             PieceType::King,
         }) {
        const Bitboard candidates = attackers & pieces[color_index(color)][type_index(type)];
        if (candidates != 0) {
            return Attacker{__builtin_ctzll(candidates), type};
        }
    }
    return {};
}

PieceType placed_piece_type_after_move(const Board& board, const Move& move) {
    if (move.is_promotion()) {
        return move.promotion;
    }
    return type_of(board.piece_at(move.from));
}

}  // namespace

int static_exchange_eval(const Board& board, const Move& move) {
    if (!move.is_capture()) {
        return 0;
    }

    const Piece moving_piece = board.piece_at(move.from);
    const Piece captured_piece = captured_piece_for(board, move);
    if (moving_piece == Piece::None || captured_piece == Piece::None) {
        return 0;
    }

    PieceBoards pieces{};
    for (const Color color : {Color::White, Color::Black}) {
        for (const PieceType type : {
                 PieceType::Pawn,
                 PieceType::Knight,
                 PieceType::Bishop,
                 PieceType::Rook,
                 PieceType::Queen,
                 PieceType::King,
             }) {
            pieces[color_index(color)][type_index(type)] = board.pieces(color, type);
        }
    }

    const Color moving_color = board.side_to_move();
    const Color captured_color = opposite(moving_color);
    const Square captured_square = captured_square_for(board, move);
    const PieceType placed_type = placed_piece_type_after_move(board, move);

    Bitboard occupancy = board.occupancy();
    remove_piece(pieces, moving_color, type_of(moving_piece), move.from);
    remove_piece(pieces, captured_color, type_of(captured_piece), captured_square);
    add_piece(pieces, moving_color, placed_type, move.to);
    occupancy &= ~square_bb(move.from);
    occupancy &= ~square_bb(captured_square);
    occupancy |= square_bb(move.to);

    std::array<int, 32> gains{};
    int depth = 0;
    gains[0] = material_value(type_of(captured_piece)) + promotion_gain(board, move);

    Color side = captured_color;
    PieceType target_piece_type = placed_type;
    while (depth + 1 < static_cast<int>(gains.size())) {
        const Bitboard attackers = attackers_to(pieces, occupancy, move.to, side);
        const Attacker attacker = least_valuable_attacker(pieces, attackers, side);
        if (!is_valid_square(attacker.square)) {
            break;
        }

        ++depth;
        gains[depth] = material_value(target_piece_type) - gains[depth - 1];

        remove_piece(pieces, opposite(side), target_piece_type, move.to);
        remove_piece(pieces, side, attacker.type, attacker.square);
        add_piece(pieces, side, attacker.type, move.to);
        occupancy &= ~square_bb(attacker.square);
        occupancy |= square_bb(move.to);

        target_piece_type = attacker.type;
        side = opposite(side);
    }

    while (depth > 0) {
        gains[depth - 1] = -std::max(-gains[depth - 1], gains[depth]);
        --depth;
    }

    return gains[0];
}

}  // namespace chess::engine
