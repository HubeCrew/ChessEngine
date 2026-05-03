#include "chess/engine/position_suite.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "chess/core/fen.h"

namespace chess::engine {

namespace {

std::string trim(std::string_view value) {
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])) != 0) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])) != 0) {
        --last;
    }
    return std::string(value.substr(first, last - first));
}

std::vector<std::string> split_epd_operations(std::string_view value) {
    std::vector<std::string> operations;
    std::string current;
    bool in_quote = false;
    bool escaped = false;

    for (const char ch : value) {
        if (escaped) {
            current.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\' && in_quote) {
            current.push_back(ch);
            escaped = true;
            continue;
        }
        if (ch == '"') {
            in_quote = !in_quote;
            current.push_back(ch);
            continue;
        }
        if (ch == ';' && !in_quote) {
            std::string operation = trim(current);
            if (!operation.empty()) {
                operations.push_back(std::move(operation));
            }
            current.clear();
            continue;
        }
        current.push_back(ch);
    }

    std::string operation = trim(current);
    if (!operation.empty()) {
        operations.push_back(std::move(operation));
    }
    return operations;
}

std::string parse_epd_string(std::string_view value) {
    const std::string trimmed = trim(value);
    if (trimmed.size() < 2 || trimmed.front() != '"' || trimmed.back() != '"') {
        return trimmed;
    }

    std::string result;
    result.reserve(trimmed.size() - 2);
    bool escaped = false;
    for (std::size_t index = 1; index + 1 < trimmed.size(); ++index) {
        const char ch = trimmed[index];
        if (escaped) {
            result.push_back(ch);
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else {
            result.push_back(ch);
        }
    }
    if (escaped) {
        result.push_back('\\');
    }
    return result;
}

std::vector<std::string> parse_best_moves(std::string_view value) {
    std::vector<std::string> moves;
    std::istringstream input{parse_epd_string(value)};
    std::string move;
    while (input >> move) {
        if (!move.empty() && move.back() == ',') {
            move.pop_back();
        }
        if (!move.empty()) {
            moves.push_back(std::move(move));
        }
    }
    return moves;
}

std::unordered_map<std::string, std::string> parse_operations(std::string_view value) {
    std::unordered_map<std::string, std::string> operations;
    for (const std::string& operation : split_epd_operations(value)) {
        std::istringstream input(operation);
        std::string opcode;
        input >> opcode;
        if (opcode.empty()) {
            continue;
        }

        std::string operand;
        std::getline(input, operand);
        operations[opcode] = trim(operand);
    }
    return operations;
}

SuitePosition parse_epd_line(std::string_view line, const std::filesystem::path& path, int line_number) {
    std::istringstream input{std::string(line)};
    std::array<std::string, 4> fen_fields{};
    for (std::string& field : fen_fields) {
        if (!(input >> field)) {
            throw std::runtime_error(path.string() + ":" + std::to_string(line_number) + ": expected 4 EPD FEN fields");
        }
    }

    std::string operations_text;
    std::getline(input, operations_text);
    const std::unordered_map<std::string, std::string> operations = parse_operations(operations_text);

    int halfmove_clock = 0;
    int fullmove_number = 1;
    int depth = 1;

    if (const auto found = operations.find("hmvc"); found != operations.end()) {
        halfmove_clock = std::stoi(found->second);
    }
    if (const auto found = operations.find("fmvn"); found != operations.end()) {
        fullmove_number = std::stoi(found->second);
    }
    if (const auto found = operations.find("acd"); found != operations.end()) {
        depth = std::stoi(found->second);
    }
    if (depth <= 0) {
        throw std::runtime_error(path.string() + ":" + std::to_string(line_number) + ": acd depth must be positive");
    }

    SuitePosition position;
    position.fen = fen_fields[0] + ' ' + fen_fields[1] + ' ' + fen_fields[2] + ' ' + fen_fields[3]
        + ' ' + std::to_string(halfmove_clock) + ' ' + std::to_string(fullmove_number);
    position.depth = depth;
    position.id = operations.contains("id") ? parse_epd_string(operations.at("id")) : path.stem().string() + "-" + std::to_string(line_number);
    position.theme = operations.contains("theme") ? parse_epd_string(operations.at("theme")) : "epd";
    position.description = operations.contains("c0") ? parse_epd_string(operations.at("c0")) : "";
    if (const auto found = operations.find("bm"); found != operations.end()) {
        position.expected_best_moves = parse_best_moves(found->second);
    }

    (void)board_from_fen(position.fen);
    return position;
}

constexpr std::array<BenchmarkPosition, 8> kBenchmarkPositions{{
    {
        "startpos",
        "Initial position",
        kStartFen,
        5,
    },
    {
        "kiwipete",
        "Castling, pins, discovered attacks",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        4,
    },
    {
        "endgame-pawns",
        "Endgame pawn races and rook checks",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        5,
    },
    {
        "promotion-race",
        "Promotions, castling rights, and exposed kings",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        4,
    },
    {
        "middlegame-tension",
        "Symmetric middlegame with bishop pins and castled kings",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        4,
    },
    {
        "queen-pressure",
        "Open files and loose heavy pieces",
        "2r2rk1/pp2qppp/2n1bn2/3p4/3P4/2NBPN2/PPQ2PPP/2RR2K1 w - - 0 13",
        4,
    },
    {
        "minor-piece-endgame",
        "Minor pieces and passed pawns",
        "8/2k5/2p2p2/p1Pp1Pp1/P2P2P1/2K5/8/8 w - - 0 42",
        6,
    },
    {
        "king-safety",
        "King shelter and attacking chances",
        "r2q1rk1/ppp2ppp/2n2n2/3bp3/3P4/2PBPN2/PP3PPP/RNBQ1RK1 w - - 0 8",
        4,
    },
}};

constexpr std::array<TacticalPosition, 50> kTacticalPositions{{
    {
        "mate-in-one-back-rank",
        "mate-in-1",
        "Protected rook delivers immediate mate on f8",
        "6k1/6pp/8/2B5/8/8/6PP/5RK1 w - - 0 1",
        2,
        {"f1f8", {}, {}, {}},
        1,
    },
    {
        "hanging-queen",
        "hanging queen",
        "Rook wins an undefended queen on the h-file",
        "4k3/7q/8/8/8/8/8/4K2R w - - 0 1",
        1,
        {"h1h7", {}, {}, {}},
        1,
    },
    {
        "promotion-choice",
        "promotion",
        "Promote the advanced pawn to a queen",
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
        1,
        {"a7a8q", {}, {}, {}},
        1,
    },
    {
        "free-rook",
        "winning capture",
        "Queen wins a loose rook without compensation",
        "4k3/7r/8/8/8/8/8/4K2Q w - - 0 1",
        1,
        {"h1h7", {}, {}, {}},
        1,
    },
    {
        "knight-fork",
        "fork",
        "Knight fork checks the king and attacks the queen",
        "2q1k3/8/8/1N6/8/8/8/4K3 w - - 0 1",
        2,
        {"b5d6", {}, {}, {}},
        1,
    },
    {
        "passed-pawn-push",
        "passed pawn",
        "Convert the advanced passer into a queen",
        "7k/4P3/8/8/8/8/8/4K3 w - - 0 1",
        1,
        {"e7e8q", {}, {}, {}},
        1,
    },
    {
        "sparkchess-mate-in-two",
        "mate-in-2",
        "Queen sacrifice forces Bxf6 mate",
        "r1bq2r1/b4pk1/p1pp1p2/1p2pP2/1P2P1PB/3P4/1PPQ2P1/R3K2R w - - 0 1",
        5,
        {"d2h6", {}, {}, {}},
        1,
    },
    {
        "queen-mating-net",
        "mate-net",
        "Queen and bishop coordinate a forced king hunt",
        "6k1/6pp/7Q/8/8/3B4/6PP/6K1 w - - 0 1",
        2,
        {"h6e6", "h6h7", "d3c4", {}},
        3,
    },
    {
        "black-rook-back-rank-mate",
        "mate-in-1",
        "Black rook mates on f1 with bishop support",
        "5rk1/6pp/8/8/2b5/8/6PP/6K1 b - - 0 1",
        2,
        {"f8f1", {}, {}, {}},
        1,
    },
    {
        "bishop-wins-queen",
        "hanging queen",
        "Bishop wins an undefended queen on h6",
        "4k3/8/7q/8/8/8/8/2B1K3 w - - 0 1",
        1,
        {"c1h6", {}, {}, {}},
        1,
    },
    {
        "knight-wins-queen",
        "hanging queen",
        "Knight captures a loose queen on h4",
        "4k3/8/8/8/7q/5N2/8/4K3 w - - 0 1",
        1,
        {"f3h4", {}, {}, {}},
        1,
    },
    {
        "rook-wins-queen",
        "hanging queen",
        "Rook lifts up the a-file to win a queen",
        "4k3/q7/8/8/8/8/8/R3K3 w - - 0 1",
        1,
        {"a1a7", {}, {}, {}},
        1,
    },
    {
        "queen-trades-up",
        "hanging queen",
        "Queen wins the opposing queen on h7",
        "4k3/7q/8/8/8/8/8/4K2Q w - - 0 1",
        1,
        {"h1h7", {}, {}, {}},
        1,
    },
    {
        "pawn-wins-queen",
        "hanging queen",
        "Advanced pawn captures a queen on d6",
        "4k3/8/3q4/4P3/8/8/8/4K3 w - - 0 1",
        1,
        {"e5d6", {}, {}, {}},
        1,
    },
    {
        "black-rook-wins-queen",
        "hanging queen",
        "Black rook wins a queen on h2",
        "4k2r/8/8/8/8/8/7Q/4K3 b - - 0 1",
        1,
        {"h8h2", {}, {}, {}},
        1,
    },
    {
        "black-bishop-wins-queen",
        "hanging queen",
        "Black bishop wins a queen on h3",
        "2b1k3/8/8/8/8/7Q/8/4K3 b - - 0 1",
        1,
        {"c8h3", {}, {}, {}},
        1,
    },
    {
        "black-knight-wins-queen",
        "hanging queen",
        "Black knight captures a loose queen on h5",
        "4k3/8/5n2/7Q/8/8/8/4K3 b - - 0 1",
        1,
        {"f6h5", {}, {}, {}},
        1,
    },
    {
        "black-pawn-wins-queen",
        "hanging queen",
        "Black pawn captures a queen on d3",
        "4k3/8/8/8/4p3/3Q4/8/4K3 b - - 0 1",
        1,
        {"e4d3", {}, {}, {}},
        1,
    },
    {
        "rook-wins-rook",
        "winning capture",
        "Rook wins a loose rook on a7",
        "4k3/r7/8/8/8/8/8/R3K3 w - - 0 1",
        1,
        {"a1a7", {}, {}, {}},
        1,
    },
    {
        "bishop-wins-rook",
        "winning capture",
        "Bishop wins a rook on g5",
        "4k3/8/8/6r1/8/8/8/2B1K3 w - - 0 1",
        1,
        {"c1g5", {}, {}, {}},
        1,
    },
    {
        "knight-wins-rook",
        "winning capture",
        "Knight captures a rook on h4",
        "4k3/8/8/8/7r/5N2/8/4K3 w - - 0 1",
        1,
        {"f3h4", {}, {}, {}},
        1,
    },
    {
        "pawn-wins-rook",
        "winning capture",
        "Pawn captures a rook on d6",
        "4k3/8/3r4/4P3/8/8/8/4K3 w - - 0 1",
        1,
        {"e5d6", {}, {}, {}},
        1,
    },
    {
        "queen-wins-bishop",
        "winning capture",
        "Queen wins a bishop on h7",
        "4k3/7b/8/8/8/8/8/4K2Q w - - 0 1",
        1,
        {"h1h7", {}, {}, {}},
        1,
    },
    {
        "rook-wins-bishop",
        "winning capture",
        "Rook wins a bishop on h7",
        "4k3/7b/8/8/8/8/8/4K2R w - - 0 1",
        1,
        {"h1h7", {}, {}, {}},
        1,
    },
    {
        "black-queen-wins-rook",
        "winning capture",
        "Black queen wins a rook on h2",
        "4k2q/8/8/8/8/8/7R/4K3 b - - 0 1",
        1,
        {"h8h2", {}, {}, {}},
        1,
    },
    {
        "black-rook-wins-rook",
        "winning capture",
        "Black rook wins a rook on a2",
        "r3k3/8/8/8/8/8/R7/4K3 b - - 0 1",
        1,
        {"a8a2", {}, {}, {}},
        1,
    },
    {
        "black-bishop-wins-rook",
        "winning capture",
        "Black bishop wins a rook on h3",
        "2b1k3/8/8/8/8/7R/8/4K3 b - - 0 1",
        1,
        {"c8h3", {}, {}, {}},
        1,
    },
    {
        "b-pawn-promotes",
        "promotion",
        "Promote the b-pawn to a queen",
        "7k/1P6/8/8/8/8/8/4K3 w - - 0 1",
        1,
        {"b7b8q", {}, {}, {}},
        1,
    },
    {
        "c-pawn-promotes",
        "promotion",
        "Promote the c-pawn to a queen",
        "7k/2P5/8/8/8/8/8/4K3 w - - 0 1",
        1,
        {"c7c8q", {}, {}, {}},
        1,
    },
    {
        "g-pawn-promotes",
        "promotion",
        "Promote the g-pawn to a queen",
        "k7/6P1/8/8/8/8/8/4K3 w - - 0 1",
        1,
        {"g7g8q", {}, {}, {}},
        1,
    },
    {
        "capture-promotion",
        "promotion",
        "Pawn captures into promotion with check pressure",
        "k6r/6P1/8/8/8/8/8/4K3 w - - 0 1",
        1,
        {"g7h8q", {}, {}, {}},
        1,
    },
    {
        "black-a-pawn-promotes",
        "promotion",
        "Black promotes the a-pawn to a queen",
        "4k3/8/8/8/8/8/p7/7K b - - 0 1",
        1,
        {"a2a1q", {}, {}, {}},
        1,
    },
    {
        "black-h-pawn-promotes",
        "promotion",
        "Black promotes the h-pawn to a queen",
        "4k3/8/8/8/8/8/7p/K7 b - - 0 1",
        1,
        {"h2h1q", {}, {}, {}},
        1,
    },
    {
        "black-d-pawn-promotes",
        "promotion",
        "Black promotes the d-pawn to a queen",
        "4k3/8/8/8/8/8/3p4/7K b - - 0 1",
        1,
        {"d2d1q", {}, {}, {}},
        1,
    },
    {
        "black-capture-promotion",
        "promotion",
        "Black captures into promotion",
        "4k3/8/8/8/8/8/6p1/5R1K b - - 0 1",
        1,
        {"g2f1q", {}, {}, {}},
        1,
    },
    {
        "queen-fork-king-rook",
        "fork",
        "Queen check forks king and rook",
        "4k2r/8/8/8/8/8/8/4K2Q w - - 0 1",
        2,
        {"h1e4", "h1h8", {}, {}},
        2,
    },
    {
        "knight-fork-king-rook",
        "fork",
        "Knight check forks king and rook",
        "2r1k3/8/8/1N6/8/8/8/4K3 w - - 0 1",
        2,
        {"b5d6", {}, {}, {}},
        1,
    },
    {
        "black-knight-fork",
        "fork",
        "Black knight fork picks up a rook",
        "4k3/8/8/8/1n6/8/8/2R1K3 b - - 0 1",
        2,
        {"b4d3", {}, {}, {}},
        1,
    },
    {
        "only-winning-capture",
        "winning capture",
        "King safely captures a nearby queen",
        "4k3/8/8/8/8/8/3q4/4K3 w - - 0 1",
        1,
        {"e1d2", {}, {}, {}},
        1,
    },
    {
        "black-king-captures-queen",
        "winning capture",
        "Black king safely captures a nearby queen",
        "4k3/3Q4/8/8/8/8/8/4K3 b - - 0 1",
        1,
        {"e8d7", {}, {}, {}},
        1,
    },
    {
        "rook-check-wins-queen",
        "check",
        "King captures the checking queen",
        "4k3/8/8/8/8/8/4q3/4K2R w - - 0 1",
        1,
        {"e1e2", {}, {}, {}},
        1,
    },
    {
        "black-rook-check-wins-queen",
        "check",
        "Black king captures the checking queen",
        "4k2r/4Q3/8/8/8/8/8/4K3 b - - 0 1",
        1,
        {"e8e7", {}, {}, {}},
        1,
    },
    {
        "advanced-pawn-captures-knight",
        "pawn tactic",
        "Advanced pawn captures a loose knight",
        "4k3/8/3n4/4P3/8/8/8/4K3 w - - 0 1",
        1,
        {"e5d6", {}, {}, {}},
        1,
    },
    {
        "black-advanced-pawn-captures-knight",
        "pawn tactic",
        "Black advanced pawn captures a loose knight",
        "4k3/8/8/8/4p3/3N4/8/4K3 b - - 0 1",
        1,
        {"e4d3", {}, {}, {}},
        1,
    },
    {
        "rook-captures-pinned-piece",
        "loose piece",
        "Rook captures a loose knight",
        "4k3/7n/8/8/8/8/8/4K2R w - - 0 1",
        1,
        {"h1h7", {}, {}, {}},
        1,
    },
    {
        "black-rook-captures-pinned-piece",
        "loose piece",
        "Black rook captures a loose knight",
        "4k2r/8/8/8/8/8/7N/4K3 b - - 0 1",
        1,
        {"h8h2", {}, {}, {}},
        1,
    },
    {
        "queen-captures-loose-knight",
        "loose piece",
        "Queen captures a loose knight",
        "4k3/7n/8/8/8/8/8/4K2Q w - - 0 1",
        1,
        {"h1h7", {}, {}, {}},
        1,
    },
    {
        "black-queen-captures-loose-knight",
        "loose piece",
        "Black queen captures a loose knight",
        "4k2q/8/8/8/8/8/7N/4K3 b - - 0 1",
        1,
        {"h8h2", {}, {}, {}},
        1,
    },
    {
        "bishop-captures-loose-knight",
        "loose piece",
        "Bishop captures a loose knight",
        "4k3/8/8/6n1/8/8/8/2B1K3 w - - 0 1",
        1,
        {"c1g5", {}, {}, {}},
        1,
    },
    {
        "black-bishop-captures-loose-knight",
        "loose piece",
        "Black bishop captures a loose knight",
        "2b1k3/8/8/8/8/7N/8/4K3 b - - 0 1",
        1,
        {"c8h3", {}, {}, {}},
        1,
    },
}};

}  // namespace

std::span<const BenchmarkPosition> benchmark_positions() {
    return kBenchmarkPositions;
}

std::span<const TacticalPosition> tactical_positions() {
    return kTacticalPositions;
}

bool is_expected_best_move(const TacticalPosition& position, std::string_view move) {
    return std::find(
        position.expected_best_moves.begin(),
        position.expected_best_moves.begin() + static_cast<std::ptrdiff_t>(position.expected_best_move_count),
        move
    ) != position.expected_best_moves.begin() + static_cast<std::ptrdiff_t>(position.expected_best_move_count);
}

bool is_expected_best_move(const SuitePosition& position, std::string_view move) {
    return std::find(position.expected_best_moves.begin(), position.expected_best_moves.end(), move)
        != position.expected_best_moves.end();
}

std::vector<SuitePosition> load_epd_suite(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open EPD suite: " + path.string());
    }

    std::vector<SuitePosition> positions;
    std::string line;
    int line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }
        positions.push_back(parse_epd_line(trimmed, path, line_number));
    }

    if (positions.empty()) {
        throw std::runtime_error("EPD suite is empty: " + path.string());
    }
    return positions;
}

}  // namespace chess::engine
