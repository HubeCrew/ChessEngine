#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "chess/core/fen.h"
#include "chess/core/move.h"
#include "chess/core/movegen.h"
#include "chess/engine/search.h"

namespace {

chess::Board parse_position(std::istringstream& input) {
    std::string token;
    input >> token;

    chess::Board board;
    if (token == "startpos") {
        board = chess::Board::start_position();
        if (input >> token && token != "moves") {
            return board;
        }
    } else if (token == "fen") {
        std::string fen;
        for (int index = 0; index < 6 && input >> token; ++index) {
            if (!fen.empty()) {
                fen.push_back(' ');
            }
            fen += token;
        }
        board = chess::board_from_fen(fen);
        if (!(input >> token) || token != "moves") {
            return board;
        }
    } else {
        return chess::Board::start_position();
    }

    while (input >> token) {
        const chess::Move move = chess::parse_uci_move(board, token);
        board.make_move(move);
    }
    return board;
}

void apply_setoption(chess::engine::Searcher& searcher, std::istringstream& input) {
    std::string token;
    std::string name;
    std::string value;

    while (input >> token) {
        if (token == "name") {
            name.clear();
            while (input >> token && token != "value") {
                if (!name.empty()) {
                    name.push_back(' ');
                }
                name += token;
            }
            if (token != "value") {
                break;
            }
            std::getline(input, value);
            if (!value.empty() && value.front() == ' ') {
                value.erase(value.begin());
            }
            break;
        }
    }

    if (name == "Hash" && !value.empty()) {
        searcher.set_hash_size_mb(static_cast<std::size_t>(std::stoul(value)));
    }
}

std::string pv_to_string(const std::vector<chess::Move>& principal_variation) {
    std::string result;
    for (const chess::Move& move : principal_variation) {
        if (!result.empty()) {
            result.push_back(' ');
        }
        result += chess::move_to_uci(move);
    }
    return result;
}

}  // namespace

int main() {
    std::cout.setf(std::ios::unitbuf);

    chess::Board board = chess::Board::start_position();
    chess::engine::Searcher searcher;

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream input(line);
        std::string command;
        input >> command;

        try {
            if (command == "uci") {
                std::cout << "id name ChessEngine 0.1\n";
                std::cout << "id author HubeKnaepkens\n";
                std::cout << "option name Hash type spin default 64 min 1 max 4096\n";
                std::cout << "uciok\n";
            } else if (command == "isready") {
                std::cout << "readyok\n";
            } else if (command == "ucinewgame") {
                board = chess::Board::start_position();
                searcher.clear();
            } else if (command == "setoption") {
                apply_setoption(searcher, input);
            } else if (command == "position") {
                board = parse_position(input);
            } else if (command == "go") {
                chess::engine::SearchLimits limits;
                std::string token;
                while (input >> token) {
                    if (token == "depth") {
                        input >> limits.depth;
                    } else if (token == "movetime") {
                        int milliseconds = 0;
                        input >> milliseconds;
                        limits.move_time = std::chrono::milliseconds(milliseconds);
                    }
                }
                const chess::engine::SearchResult result = searcher.search(board, limits);
                std::cout << "info depth " << result.depth
                          << " score cp " << result.score_centipawns
                          << " nodes " << result.nodes
                          << " nps " << result.nps
                          << " time " << result.elapsed.count();
                if (!result.principal_variation.empty()) {
                    std::cout << " pv " << pv_to_string(result.principal_variation);
                }
                std::cout << '\n';
                std::cout << "bestmove " << chess::move_to_uci(result.best_move) << '\n';
            } else if (command == "d") {
                std::cout << board.to_fen() << '\n';
            } else if (command == "quit") {
                break;
            }
        } catch (const std::exception& error) {
            std::cout << "info string error: " << error.what() << '\n';
        }
    }

    return 0;
}
