#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "chess/core/fen.h"
#include "chess/core/move.h"
#include "chess/core/movegen.h"
#include "chess/core/perft.h"

int main(int argc, char** argv) {
    try {
        int depth = 1;
        std::string fen{chess::kStartFen};

        if (argc >= 2) {
            depth = std::atoi(argv[1]);
        }
        if (argc >= 3) {
            fen.clear();
            for (int index = 2; index < argc; ++index) {
                if (!fen.empty()) {
                    fen.push_back(' ');
                }
                fen += argv[index];
            }
        }

        chess::Board board = chess::board_from_fen(fen);
        const auto moves = chess::generate_legal_moves(board);
        std::uint64_t total = 0;
        for (const chess::Move& move : moves) {
            const chess::UndoState undo = board.make_move(move);
            const std::uint64_t nodes = chess::perft(board, depth - 1);
            board.unmake_move(undo);
            total += nodes;
            std::cout << chess::move_to_uci(move) << ": " << nodes << '\n';
        }
        std::cout << "nodes " << total << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}

