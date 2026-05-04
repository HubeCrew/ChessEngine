#include <iostream>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include "chess/core/fen.h"
#include "chess/core/move.h"
#include "chess/core/movegen.h"
#include "chess/engine/evaluation.h"
#include "chess/engine/search.h"
#include "chess/engine/static_exchange.h"

namespace {

bool parse_bool(std::string_view value) {
    return value == "true" || value == "1" || value == "on";
}

chess::engine::EvalType parse_eval_type(std::string_view value) {
    if (value == "classical") {
        return chess::engine::EvalType::Classical;
    }
    if (value == "nnue") {
        return chess::engine::EvalType::Nnue;
    }
    if (value == "hybrid") {
        return chess::engine::EvalType::Hybrid;
    }
    throw std::invalid_argument("EvalType must be classical, nnue, or hybrid");
}

std::string_view eval_type_name(chess::engine::EvalType type) {
    switch (type) {
        case chess::engine::EvalType::Classical:
            return "classical";
        case chess::engine::EvalType::Nnue:
            return "nnue";
        case chess::engine::EvalType::Hybrid:
            return "hybrid";
    }
    return "classical";
}

bool is_go_option(std::string_view token) {
    return token == "depth"
        || token == "movetime"
        || token == "wtime"
        || token == "btime"
        || token == "winc"
        || token == "binc"
        || token == "movestogo"
        || token == "searchmoves";
}

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
    } else if (name == "NullMovePruning" && !value.empty()) {
        searcher.set_null_move_pruning(parse_bool(value));
    } else if (name == "SearchExtensions" && !value.empty()) {
        searcher.set_search_extensions(parse_bool(value));
    } else if (name == "EvalType" && !value.empty()) {
        searcher.set_eval_type(parse_eval_type(value));
    } else if (name == "NnueFile") {
        if (value.empty()) {
            searcher.clear_nnue();
        } else {
            std::string error;
            if (!searcher.load_nnue(std::filesystem::path{value}, &error)) {
                std::cout << "info string nnue load failed: " << error << '\n';
            } else {
                std::cout << "info string nnue loaded " << searcher.nnue_info().path.string()
                          << " hidden " << searcher.nnue_info().hidden_size << '\n';
            }
        }
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

void print_eval_trace(const chess::Board& board, const chess::engine::Searcher& searcher) {
    const chess::engine::EvalTrace trace = chess::engine::evaluate_trace_white_perspective(board);
    const bool nnue_loaded = searcher.nnue_loaded();
    std::cout << "info string eval"
              << " material " << trace.material
              << " piece_square " << trace.piece_square
              << " mobility " << trace.mobility
              << " safe_mobility " << trace.safe_mobility
              << " king_safety " << trace.king_safety
              << " threats " << trace.threats
              << " pawn_structure " << trace.pawn_structure
              << " outposts " << trace.outposts
              << " rook_files " << trace.rook_files
              << " space " << trace.space
              << " center_control " << trace.center_control
              << " bishop_quality " << trace.bishop_quality
              << " pawn_dynamics " << trace.pawn_dynamics
              << " development " << trace.development
              << " trade_context " << trace.trade_context
              << " classical_total " << trace.total
              << " selected " << eval_type_name(searcher.eval_type())
              << " nnue_loaded " << (nnue_loaded ? "true" : "false");
    if (nnue_loaded) {
        const int raw_nnue = searcher.evaluate_nnue_white_perspective(board);
        std::cout << " nnue_total " << raw_nnue
                  << " selected_total " << searcher.evaluate_white_perspective(board)
                  << " delta " << (raw_nnue - trace.total);
    } else {
        std::cout << " selected_total " << trace.total
                  << " delta 0";
    }
    std::cout << " total " << searcher.evaluate_white_perspective(board)
              << '\n';
}

void print_static_exchange(const chess::Board& board, std::istringstream& input) {
    std::string move_text;
    input >> move_text;
    chess::Board copy = board;
    const chess::Move move = chess::parse_uci_move(copy, move_text);
    std::cout << "info string see " << chess::move_to_uci(move)
              << ' ' << chess::engine::static_exchange_eval(copy, move)
              << '\n';
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
                std::cout << "option name NullMovePruning type check default true\n";
                std::cout << "option name SearchExtensions type check default true\n";
                std::cout << "option name EvalType type combo default classical var classical var nnue var hybrid\n";
                std::cout << "option name NnueFile type string default <empty>\n";
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
                std::vector<std::string> tokens;
                std::string token;
                while (input >> token) {
                    tokens.push_back(token);
                }
                for (std::size_t index = 0; index < tokens.size(); ++index) {
                    token = tokens[index];
                    if (token == "depth") {
                        if (++index < tokens.size()) {
                            limits.depth = std::stoi(tokens[index]);
                        }
                    } else if (token == "searchmoves") {
                        while (index + 1 < tokens.size() && !is_go_option(tokens[index + 1])) {
                            ++index;
                            limits.search_moves.push_back(chess::parse_uci_move(board, tokens[index]));
                        }
                    } else if (token == "movetime") {
                        if (++index < tokens.size()) {
                            limits.move_time = std::chrono::milliseconds{std::max(0, std::stoi(tokens[index]))};
                        }
                    } else if (token == "wtime") {
                        if (++index < tokens.size()) {
                            limits.white_time = std::chrono::milliseconds{std::max(0, std::stoi(tokens[index]))};
                        }
                    } else if (token == "btime") {
                        if (++index < tokens.size()) {
                            limits.black_time = std::chrono::milliseconds{std::max(0, std::stoi(tokens[index]))};
                        }
                    } else if (token == "winc") {
                        if (++index < tokens.size()) {
                            limits.white_increment = std::chrono::milliseconds{std::max(0, std::stoi(tokens[index]))};
                        }
                    } else if (token == "binc") {
                        if (++index < tokens.size()) {
                            limits.black_increment = std::chrono::milliseconds{std::max(0, std::stoi(tokens[index]))};
                        }
                    } else if (token == "movestogo") {
                        if (++index < tokens.size()) {
                            limits.moves_to_go = std::stoi(tokens[index]);
                        }
                    }
                }
                const chess::engine::SearchResult result = searcher.search(board, limits);
                std::cout << "info depth " << result.depth
                          << " score cp " << result.score_centipawns
                          << " nodes " << result.nodes
                          << " qnodes " << result.qnodes
                          << " nps " << result.nps
                          << " time " << result.elapsed.count();
                if (!result.principal_variation.empty()) {
                    std::cout << " pv " << pv_to_string(result.principal_variation);
                }
                std::cout << '\n';
                std::cout << "bestmove " << chess::move_to_uci(result.best_move) << '\n';
            } else if (command == "d") {
                std::cout << board.to_fen() << '\n';
            } else if (command == "eval") {
                print_eval_trace(board, searcher);
            } else if (command == "see") {
                print_static_exchange(board, input);
            } else if (command == "quit") {
                break;
            }
        } catch (const std::exception& error) {
            std::cout << "info string error: " << error.what() << '\n';
        }
    }

    return 0;
}
