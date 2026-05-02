#include "chess/core/perft.h"

#include "chess/core/movegen.h"

namespace chess {

std::uint64_t perft(Board& board, int depth) {
    if (depth == 0) {
        return 1;
    }

    const MoveList moves = generate_legal_moves(board);
    if (depth == 1) {
        return moves.size();
    }

    std::uint64_t nodes = 0;
    for (const Move& move : moves) {
        const UndoState undo = board.make_move(move);
        nodes += perft(board, depth - 1);
        board.unmake_move(undo);
    }
    return nodes;
}

}  // namespace chess

