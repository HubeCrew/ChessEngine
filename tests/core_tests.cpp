#include <catch2/catch_test_macros.hpp>

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

#include "chess/core/fen.h"
#include "chess/core/move.h"
#include "chess/core/movegen.h"
#include "chess/core/perft.h"

namespace {

struct PerftCase {
    const char* name;
    const char* fen;
    std::vector<std::uint64_t> expected_nodes;
};

void require_perft_sequence(const PerftCase& test_case) {
    chess::Board board = chess::board_from_fen(test_case.fen);
    for (std::size_t depth = 0; depth < test_case.expected_nodes.size(); ++depth) {
        INFO(test_case.name << " depth " << depth);
        REQUIRE(chess::perft(board, static_cast<int>(depth)) == test_case.expected_nodes[depth]);
        REQUIRE(board.to_fen() == test_case.fen);
    }
}

bool contains_move(chess::Board& board, std::string_view uci_move) {
    const chess::MoveList moves = chess::generate_legal_moves(board);
    for (const chess::Move& move : moves) {
        if (chess::move_to_uci(move) == uci_move) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST_CASE("start position FEN round trips") {
    const chess::Board board = chess::board_from_fen(chess::kStartFen);
    REQUIRE(board.to_fen() == chess::kStartFen);
}

TEST_CASE("published perft corpus is correct") {
    const std::array<PerftCase, 6> cases{{
        {
            "initial position",
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            {1, 20, 400, 8902, 197281},
        },
        {
            "kiwipete",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            {1, 48, 2039, 97862},
        },
        {
            "endgame promotions and checks",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            {1, 14, 191, 2812, 43238},
        },
        {
            "promotion race with castling rights",
            "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
            {1, 6, 264, 9467},
        },
        {
            "discovered attacks and underpromotions",
            "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            {1, 44, 1486, 62379},
        },
        {
            "middlegame sliders and king safety",
            "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
            {1, 46, 2079, 89890},
        },
    }};

    for (const PerftCase& test_case : cases) {
        require_perft_sequence(test_case);
    }
}

TEST_CASE("en-passant cannot expose own king") {
    chess::Board board = chess::board_from_fen("8/6bb/8/8/R1pP2k1/4P3/P7/K7 b - d3 0 1");
    const chess::MoveList moves = chess::generate_legal_moves(board);
    for (const chess::Move& move : moves) {
        REQUIRE(chess::move_to_uci(move) != "c4d3");
    }
}

TEST_CASE("en-passant captures update and restore full state") {
    chess::Board board = chess::board_from_fen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    const std::string original = board.to_fen();

    REQUIRE(contains_move(board, "e5d6"));
    const chess::Move move = chess::parse_uci_move(board, "e5d6");
    const chess::UndoState undo = board.make_move(move);

    REQUIRE(board.to_fen() == "4k3/8/3P4/8/8/8/8/4K3 b - - 0 1");
    board.unmake_move(undo);
    REQUIRE(board.to_fen() == original);
}

TEST_CASE("castling rights are removed by king moves, rook moves, and rook captures") {
    SECTION("king move removes both rights for that side") {
        chess::Board board = chess::board_from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        const chess::UndoState undo = board.make_move(chess::parse_uci_move(board, "e1d1"));
        REQUIRE((board.castling_rights() & (chess::WhiteKingSide | chess::WhiteQueenSide)) == 0);
        REQUIRE((board.castling_rights() & (chess::BlackKingSide | chess::BlackQueenSide))
                == (chess::BlackKingSide | chess::BlackQueenSide));
        board.unmake_move(undo);
        REQUIRE(board.to_fen() == "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    }

    SECTION("rook move removes only the matching side") {
        chess::Board board = chess::board_from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        const chess::UndoState undo = board.make_move(chess::parse_uci_move(board, "h1h2"));
        REQUIRE((board.castling_rights() & chess::WhiteKingSide) == 0);
        REQUIRE((board.castling_rights() & chess::WhiteQueenSide) != 0);
        board.unmake_move(undo);
        REQUIRE(board.to_fen() == "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    }

    SECTION("capturing a rook on its home square removes the opponent right") {
        chess::Board board = chess::board_from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        const chess::UndoState undo = board.make_move(chess::parse_uci_move(board, "a1a8"));
        REQUIRE((board.castling_rights() & chess::BlackQueenSide) == 0);
        REQUIRE((board.castling_rights() & chess::BlackKingSide) != 0);
        board.unmake_move(undo);
        REQUIRE(board.to_fen() == "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    }
}

TEST_CASE("castling legality rejects attacked transit squares") {
    chess::Board board = chess::board_from_fen("r3k2r/8/8/8/8/8/5r2/R3K2R w KQkq - 0 1");

    REQUIRE_FALSE(contains_move(board, "e1g1"));
    REQUIRE(contains_move(board, "e1c1"));
}

TEST_CASE("terminal positions distinguish checkmate and stalemate") {
    SECTION("fools mate is checkmate") {
        chess::Board board = chess::board_from_fen(
            "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3"
        );
        REQUIRE(board.in_check(chess::Color::White));
        REQUIRE(chess::generate_legal_moves(board).empty());
        REQUIRE(chess::perft(board, 1) == 0);
    }

    SECTION("boxed king is stalemate") {
        chess::Board board = chess::board_from_fen("7k/5K2/6Q1/8/8/8/8/8 b - - 0 1");
        REQUIRE_FALSE(board.in_check(chess::Color::Black));
        REQUIRE(chess::generate_legal_moves(board).empty());
        REQUIRE(chess::perft(board, 1) == 0);
    }
}

TEST_CASE("make and unmake restore the full FEN") {
    chess::Board board = chess::Board::start_position();
    const std::string original = board.to_fen();
    const chess::Move move = chess::parse_uci_move(board, "e2e4");
    const chess::UndoState undo = board.make_move(move);
    board.unmake_move(undo);
    REQUIRE(board.to_fen() == original);
}

TEST_CASE("every root legal move restores the full FEN") {
    chess::Board board = chess::board_from_fen(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    );
    const std::string original = board.to_fen();

    for (const chess::Move& move : chess::generate_legal_moves(board)) {
        const chess::UndoState undo = board.make_move(move);
        board.unmake_move(undo);
        REQUIRE(board.to_fen() == original);
    }
}

TEST_CASE("promotion moves include all choices") {
    chess::Board board = chess::board_from_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    const chess::MoveList moves = chess::generate_legal_moves(board);
    int promotions = 0;
    for (const chess::Move& move : moves) {
        if (move.from == chess::make_square(0, 6) && move.to == chess::make_square(0, 7)) {
            ++promotions;
        }
    }
    REQUIRE(promotions == 4);
}

TEST_CASE("FEN parser rejects malformed positions") {
    for (const std::string& fen : {
             "8/8/8/8/8/8/8/8 w - - 0 1",
             "4k3/8/8/8/8/8/8/4K3 x - - 0 1",
             "4k3/8/8/8/8/8/8/4K3 w KK - 0 1",
             "4k3/8/8/8/8/8/8/4K3 w - e4 0 1",
             "4k3/8/8/8/8/8/8/4K3 w - - -1 1",
             "4k3/8/8/8/8/8/8/4K3 w - - 0 0",
             "4k3/8/8/8/8/8/8/4K3 w - - 0x 1",
         }) {
        INFO(fen);
        REQUIRE_THROWS_AS(chess::board_from_fen(fen), std::invalid_argument);
    }
}
