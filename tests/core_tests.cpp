#include <catch2/catch_test_macros.hpp>

#include <string>

#include "chess/core/fen.h"
#include "chess/core/move.h"
#include "chess/core/movegen.h"
#include "chess/core/perft.h"

TEST_CASE("start position FEN round trips") {
    const chess::Board board = chess::board_from_fen(chess::kStartFen);
    REQUIRE(board.to_fen() == chess::kStartFen);
}

TEST_CASE("start position legal move count and perft are correct") {
    chess::Board board = chess::Board::start_position();
    REQUIRE(chess::generate_legal_moves(board).size() == 20);
    REQUIRE(chess::perft(board, 1) == 20);
    REQUIRE(chess::perft(board, 2) == 400);
    REQUIRE(chess::perft(board, 3) == 8902);
}

TEST_CASE("kiwipete perft covers castling and pinned pieces") {
    chess::Board board = chess::board_from_fen(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    );
    REQUIRE(chess::perft(board, 1) == 48);
    REQUIRE(chess::perft(board, 2) == 2039);
    REQUIRE(chess::perft(board, 3) == 97862);
}

TEST_CASE("en-passant cannot expose own king") {
    chess::Board board = chess::board_from_fen("8/6bb/8/8/R1pP2k1/4P3/P7/K7 b - d3 0 1");
    const chess::MoveList moves = chess::generate_legal_moves(board);
    for (const chess::Move& move : moves) {
        REQUIRE(chess::move_to_uci(move) != "c4d3");
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
