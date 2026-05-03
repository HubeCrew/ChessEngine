#include "chess/engine/static_exchange.h"

#include <algorithm>
#include <array>

#include "chess/core/attacks.h"
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

Bitboard pieces_of(const PieceBoards& pieces, Color color, PieceType type) {
    return pieces[color_index(color)][type_index(type)];
}

Bitboard attackers_to(const PieceBoards& pieces, Bitboard occupancy, Square target, Color color) {
    const Bitboard queens = pieces_of(pieces, color, PieceType::Queen);
    return (attacks::pawn_attacks(opposite(color), target) & pieces_of(pieces, color, PieceType::Pawn))
        | (attacks::knight_attacks(target) & pieces_of(pieces, color, PieceType::Knight))
        | (attacks::king_attacks(target) & pieces_of(pieces, color, PieceType::King))
        | (attacks::bishop_attacks(target, occupancy) & (pieces_of(pieces, color, PieceType::Bishop) | queens))
        | (attacks::rook_attacks(target, occupancy) & (pieces_of(pieces, color, PieceType::Rook) | queens));
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
