#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "chess/core/fen.h"
#include "chess/core/game_status.h"
#include "chess/core/move.h"
#include "chess/core/movegen.h"

namespace {

void print_usage() {
    std::cerr << "usage: chess_referee --fen <fen> [--moves <uci>...]\n";
}

std::string color_to_string(chess::Color color) {
    return color == chess::Color::White ? "white" : "black";
}

std::string winner_for(chess::GameOutcome outcome) {
    if (outcome == chess::GameOutcome::WhiteWin) {
        return "white";
    }
    if (outcome == chess::GameOutcome::BlackWin) {
        return "black";
    }
    return "none";
}

void print_status(bool ok, const chess::Board& board, const chess::GameStatus& status, int plies) {
    std::cout << "ok=" << (ok ? "true" : "false") << '\n';
    std::cout << "outcome=" << chess::to_string(status.outcome) << '\n';
    std::cout << "reason=" << chess::to_string(status.reason) << '\n';
    std::cout << "winner=" << winner_for(status.outcome) << '\n';
    std::cout << "fen=" << board.to_fen() << '\n';
    std::cout << "side=" << color_to_string(board.side_to_move()) << '\n';
    std::cout << "ply=" << plies << '\n';
    std::cout << "legal_moves=" << status.legal_moves << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    std::string fen;
    std::vector<std::string> moves;

    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--fen") {
            if (++index >= argc) {
                print_usage();
                return 2;
            }
            fen = argv[index];
        } else if (arg == "--moves") {
            while (++index < argc) {
                moves.emplace_back(argv[index]);
            }
            break;
        } else {
            print_usage();
            return 2;
        }
    }

    if (fen.empty()) {
        print_usage();
        return 2;
    }

    try {
        chess::Board board = chess::board_from_fen(fen);
        std::vector<std::uint64_t> repetition_history{board.hash_key()};

        for (std::size_t index = 0; index < moves.size(); ++index) {
            const chess::Color moving_side = board.side_to_move();
            try {
                const chess::Move move = chess::parse_uci_move(board, moves[index]);
                board.make_move(move);
                repetition_history.push_back(board.hash_key());
            } catch (const std::exception& error) {
                const chess::GameStatus status{
                    moving_side == chess::Color::White ? chess::GameOutcome::BlackWin : chess::GameOutcome::WhiteWin,
                    chess::GameReason::None,
                    static_cast<int>(chess::generate_legal_moves(board).size()),
                };
                print_status(false, board, status, static_cast<int>(index));
                std::cout << "illegal_move=" << moves[index] << '\n';
                std::cout << "error=" << error.what() << '\n';
                return 1;
            }
        }

        chess::Board status_board = board;
        const chess::GameStatus status = chess::adjudicate(status_board, repetition_history);
        print_status(true, board, status, static_cast<int>(moves.size()));
        return 0;
    } catch (const std::exception& error) {
        std::cout << "ok=false\n";
        std::cout << "outcome=ongoing\n";
        std::cout << "reason=none\n";
        std::cout << "winner=none\n";
        std::cout << "error=" << error.what() << '\n';
        return 1;
    }
}
