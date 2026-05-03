#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

#include "chess/core/fen.h"
#include "chess/core/movegen.h"
#include "chess/engine/evaluation.h"
#include "chess/engine/position_suite.h"
#include "chess/engine/search.h"
#include "chess/engine/static_exchange.h"
#include "chess/engine/transposition_table.h"

namespace {

bool is_legal_best_move(chess::Board& board, const chess::Move& best_move) {
    const chess::MoveList moves = chess::generate_legal_moves(board);
    return std::any_of(moves.begin(), moves.end(), [&](const chess::Move& move) {
        return move.from == best_move.from
            && move.to == best_move.to
            && move.promotion == best_move.promotion;
    });
}

bool contains_uci_move(const chess::MoveList& moves, std::string_view uci) {
    return std::any_of(moves.begin(), moves.end(), [&](const chess::Move& move) {
        return chess::move_to_uci(move) == uci;
    });
}

chess::Move legal_move_by_uci(chess::Board& board, const std::string& uci) {
    const chess::MoveList moves = chess::generate_legal_moves(board);
    const auto found = std::find_if(moves.begin(), moves.end(), [&](const chess::Move& move) {
        return chess::move_to_uci(move) == uci;
    });
    REQUIRE(found != moves.end());
    return *found;
}

bool trusted_move_gives_check(chess::Board board, const chess::Move& move) {
    const chess::Color moving_side = board.side_to_move();
    const chess::UndoState undo = board.make_move(move);
    const bool gives_check = board.in_check(chess::opposite(moving_side));
    board.unmake_move(undo);
    return gives_check;
}

}  // namespace

TEST_CASE("hash is restored after every root move") {
    chess::Board board = chess::board_from_fen(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    );
    const std::uint64_t original_hash = board.hash_key();

    REQUIRE(original_hash == board.recompute_hash());
    for (const chess::Move& move : chess::generate_legal_moves(board)) {
        const chess::UndoState undo = board.make_move(move);
        REQUIRE(board.hash_key() == board.recompute_hash());
        board.unmake_move(undo);
        REQUIRE(board.hash_key() == original_hash);
        REQUIRE(board.hash_key() == board.recompute_hash());
    }
}

TEST_CASE("hash changes for side, castling, and en-passant state") {
    const auto base = chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1").hash_key();
    REQUIRE(chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 b - - 0 1").hash_key() != base);
    REQUIRE(chess::board_from_fen("4k2r/8/8/8/8/8/8/4K2R w Kk - 0 1").hash_key() != base);
    REQUIRE(chess::board_from_fen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1").hash_key() != base);
}

TEST_CASE("transposition table stores and probes bounds") {
    chess::engine::TranspositionTable table{1};
    const chess::Move move{chess::make_square(4, 1), chess::make_square(4, 3), chess::PieceType::None, chess::DoublePawnPush};
    const std::uint64_t key = 0x123456789abcdef0ULL;

    table.store(key, 5, 42, chess::engine::Bound::Exact, move);
    const chess::engine::TranspositionEntry* entry = table.probe(key);
    REQUIRE(entry != nullptr);
    REQUIRE(entry->depth == 5);
    REQUIRE(entry->score == 42);
    REQUIRE(entry->bound == chess::engine::Bound::Exact);
    REQUIRE(entry->best_move == move);
    REQUIRE(table.probe(key ^ 1ULL) == nullptr);

    table.store(key, 3, 10, chess::engine::Bound::Upper, {});
    entry = table.probe(key);
    REQUIRE(entry != nullptr);
    REQUIRE(entry->depth == 5);
    REQUIRE(entry->score == 42);
}

TEST_CASE("search returns legal moves and principal variation") {
    chess::Board board = chess::Board::start_position();
    chess::engine::Searcher searcher;
    searcher.set_hash_size_mb(1);

    const chess::engine::SearchResult result = searcher.search(board, chess::engine::SearchLimits{3});

    REQUIRE(result.depth == 3);
    REQUIRE(result.nodes > 0);
    REQUIRE(result.nps > 0);
    REQUIRE(is_legal_best_move(board, result.best_move));
    REQUIRE_FALSE(result.principal_variation.empty());
    REQUIRE(result.principal_variation.front() == result.best_move);
}

TEST_CASE("search keeps mate-in-one tactical behavior") {
    chess::Board board = chess::board_from_fen("6k1/6pp/8/2B5/8/8/6PP/5RK1 w - - 0 1");
    chess::engine::Searcher searcher;

    const chess::engine::SearchResult result = searcher.search(board, chess::engine::SearchLimits{2});

    REQUIRE(chess::move_to_uci(result.best_move) == "f1f8");
}

TEST_CASE("pseudo-legal pinned moves are filtered before they become legal search moves") {
    chess::Board board = chess::board_from_fen("4k3/4n3/8/5Q2/8/8/P7/K3R3 b - - 0 1");

    REQUIRE(contains_uci_move(chess::generate_pseudo_legal_moves(board), "e7f5"));
    REQUIRE_FALSE(contains_uci_move(chess::generate_legal_moves(board), "e7f5"));
}

TEST_CASE("search skips illegal pinned captures during child quiescence") {
    chess::Board board = chess::board_from_fen("4k3/4n3/8/5Q2/8/8/P7/K3R3 w - - 0 1");
    chess::engine::Searcher searcher;
    searcher.set_hash_size_mb(1);

    const chess::engine::SearchResult result = searcher.search(board, chess::engine::SearchLimits{1});

    REQUIRE(result.depth == 1);
    REQUIRE(is_legal_best_move(board, result.best_move));
    REQUIRE(result.score_centipawns > 700);
    REQUIRE(result.diagnostics.illegal_pseudo_moves > 0);
    REQUIRE(board.hash_key() == board.recompute_hash());
}

TEST_CASE("search detects mate when the mated side has pseudo-legal evasions only") {
    chess::Board mated = chess::board_from_fen("5Rk1/6pp/8/2B5/8/8/6PP/6K1 b - - 1 1");

    REQUIRE(mated.in_check(chess::Color::Black));
    REQUIRE(contains_uci_move(chess::generate_pseudo_legal_moves(mated), "g8f8"));
    REQUIRE(chess::generate_legal_moves(mated).empty());

    chess::Board before_mate = chess::board_from_fen("6k1/6pp/8/2B5/8/8/6PP/5RK1 w - - 0 1");
    chess::engine::Searcher searcher;
    const chess::engine::SearchResult result = searcher.search(before_mate, chess::engine::SearchLimits{2});

    REQUIRE(chess::move_to_uci(result.best_move) == "f1f8");
}

TEST_CASE("direct check detection matches make-unmake oracle") {
    for (const char* fen : {
             "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
             "4k3/8/8/8/8/2N5/8/4K3 w - - 0 1",
             "4k3/8/8/8/8/8/4B3/R3K3 w Q - 0 1",
             "5k2/8/8/8/8/8/8/4K2R w K - 0 1",
             "4k3/8/8/3pP3/8/8/8/4R2K w - d6 0 1",
             "4k2r/6P1/8/8/8/8/8/K7 w - - 0 1",
         }) {
        chess::Board board = chess::board_from_fen(fen);
        INFO(fen);
        for (const chess::Move& move : chess::generate_legal_moves(board)) {
            INFO(chess::move_to_uci(move));
            REQUIRE(chess::engine::move_gives_check(board, move) == trusted_move_gives_check(board, move));
            REQUIRE(board.hash_key() == board.recompute_hash());
        }
    }
}

TEST_CASE("hash table can be resized and cleared through searcher") {
    chess::engine::Searcher searcher;
    searcher.set_hash_size_mb(2);
    REQUIRE(searcher.hash_size_mb() == 2);
    searcher.set_search_extensions(false);
    REQUIRE_FALSE(searcher.search_extensions());
    searcher.set_search_extensions(true);
    REQUIRE(searcher.search_extensions());
    searcher.clear();
    REQUIRE(searcher.hash_size_mb() == 2);
    REQUIRE(searcher.search_extensions());
}

TEST_CASE("evaluation scores king-only positions near equal") {
    const chess::Board board = chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    REQUIRE(std::abs(chess::engine::evaluate_white_perspective(board)) <= 5);
}

TEST_CASE("evaluation rewards extra material") {
    const chess::Board board = chess::board_from_fen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
    REQUIRE(chess::engine::evaluate_white_perspective(board) > 450);
}

TEST_CASE("evaluation mirrors color and board orientation") {
    const chess::Board white_knight = chess::board_from_fen("4k3/8/8/8/3N4/8/8/4K3 w - - 0 1");
    const chess::Board black_knight = chess::board_from_fen("4k3/8/8/3n4/8/8/8/4K3 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(white_knight)
            == -chess::engine::evaluate_white_perspective(black_knight));
}

TEST_CASE("evaluation rewards bishop pair") {
    const chess::Board bishop_pair = chess::board_from_fen("4k3/8/8/8/8/8/8/2BBK3 w - - 0 1");
    const chess::Board bishop_and_knight = chess::board_from_fen("4k3/8/8/8/8/8/8/2BNK3 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(bishop_pair)
            > chess::engine::evaluate_white_perspective(bishop_and_knight));
}

TEST_CASE("evaluation rewards passed pawns over stopped pawns") {
    const chess::Board passed = chess::board_from_fen("4k3/p7/8/4P3/8/8/8/4K3 w - - 0 1");
    const chess::Board stopped = chess::board_from_fen("4k3/8/3p4/4P3/8/8/8/4K3 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(passed)
            > chess::engine::evaluate_white_perspective(stopped));
}

TEST_CASE("evaluation rewards clear and supported passed pawns in endgames") {
    const chess::Board clear_supported = chess::board_from_fen("7k/8/4P3/3P4/8/8/8/4K3 w - - 0 1");
    const chess::Board blockaded = chess::board_from_fen("7k/4p3/4P3/3P4/8/8/8/4K3 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(clear_supported)
            > chess::engine::evaluate_white_perspective(blockaded));
}

TEST_CASE("evaluation recognizes king races for advanced passed pawns") {
    const chess::Board outside_square = chess::board_from_fen("7k/8/8/4P3/8/8/8/4K3 w - - 0 1");
    const chess::Board inside_square = chess::board_from_fen("8/8/8/4P1k1/8/8/8/4K3 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(outside_square)
            > chess::engine::evaluate_white_perspective(inside_square));
}

TEST_CASE("evaluation penalizes doubled isolated pawns") {
    const chess::Board connected = chess::board_from_fen("4k3/8/8/8/8/8/2PP4/4K3 w - - 0 1");
    const chess::Board doubled = chess::board_from_fen("4k3/8/8/8/8/2P5/2P5/4K3 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(connected)
            > chess::engine::evaluate_white_perspective(doubled));
}

TEST_CASE("evaluation rewards king centralization in endgames") {
    const chess::Board central = chess::board_from_fen("7k/8/8/8/3K4/8/8/8 w - - 0 1");
    const chess::Board corner = chess::board_from_fen("7k/8/8/8/8/8/8/K7 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(central)
            > chess::engine::evaluate_white_perspective(corner));
}

TEST_CASE("evaluation rewards intact castled king shelter") {
    const chess::Board sheltered = chess::board_from_fen("6k1/5ppp/8/8/8/8/5PPP/6K1 w - - 0 1");
    const chess::Board exposed = chess::board_from_fen("6k1/5ppp/8/8/5PPP/8/8/6K1 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(sheltered)
            > chess::engine::evaluate_white_perspective(exposed));
}

TEST_CASE("evaluation penalizes open files near the king") {
    const chess::Board closed_file = chess::board_from_fen("k4r2/8/8/8/8/8/5PPP/6K1 w - - 0 1");
    const chess::Board open_file = chess::board_from_fen("k4r2/8/8/8/8/8/P5PP/6K1 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(closed_file)
            > chess::engine::evaluate_white_perspective(open_file));
}

TEST_CASE("evaluation recognizes coordinated attacks near the king") {
    const chess::Board quiet = chess::board_from_fen("kn6/8/n6b/q7/8/8/5PPP/6K1 w - - 0 1");
    const chess::Board attacked = chess::board_from_fen("k3r3/8/5n2/2b3q1/8/8/5PPP/6K1 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(quiet)
            > chess::engine::evaluate_white_perspective(attacked));
}

TEST_CASE("evaluation rewards attacks on loose major pieces") {
    const chess::Board attacked_queen = chess::board_from_fen("k7/8/8/4q3/6N1/8/8/7K w - - 0 1");
    const chess::Board safe_queen = chess::board_from_fen("k7/8/8/4q3/8/8/6N1/7K w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(attacked_queen)
            > chess::engine::evaluate_white_perspective(safe_queen));
}

TEST_CASE("evaluation rewards pawn attacks on higher-value pieces") {
    const chess::Board pawn_attacks_rook = chess::board_from_fen("k7/8/8/4r3/3P4/8/8/7K w - - 0 1");
    const chess::Board pawn_misses_rook = chess::board_from_fen("k7/8/8/4r3/2P5/8/8/7K w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(pawn_attacks_rook)
            > chess::engine::evaluate_white_perspective(pawn_misses_rook));
}

TEST_CASE("king safety and threats mirror by color") {
    const chess::Board white_attack = chess::board_from_fen("6k1/5ppp/8/6Q1/2B5/5N2/8/6K1 w - - 0 1");
    const chess::Board black_attack = chess::board_from_fen("6k1/8/5n2/2b5/6q1/8/5PPP/6K1 w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(white_attack)
            == -chess::engine::evaluate_white_perspective(black_attack));
}

TEST_CASE("search prefers winning free queen material") {
    chess::Board board = chess::board_from_fen("4k3/7q/8/8/8/8/8/4K2R w - - 0 1");
    chess::engine::Searcher searcher;

    const chess::engine::SearchResult result = searcher.search(board, chess::engine::SearchLimits{1});

    REQUIRE(chess::move_to_uci(result.best_move) == "h1h7");
}

TEST_CASE("static exchange evaluation scores free captures") {
    chess::Board board = chess::board_from_fen("4k3/8/8/8/8/8/4q3/4R2K w - - 0 1");
    const chess::Move move = legal_move_by_uci(board, "e1e2");

    REQUIRE(chess::engine::static_exchange_eval(board, move) == 900);
    REQUIRE(board.hash_key() == board.recompute_hash());
}

TEST_CASE("static exchange evaluation detects defended material losses") {
    chess::Board board = chess::board_from_fen("k3r3/8/8/4p3/8/8/4Q3/K7 w - - 0 1");
    const chess::Move move = legal_move_by_uci(board, "e2e5");

    REQUIRE(chess::engine::static_exchange_eval(board, move) < 0);
    REQUIRE(board.hash_key() == board.recompute_hash());
}

TEST_CASE("static exchange evaluation follows recapture chains") {
    chess::Board board = chess::board_from_fen("k3r3/8/8/8/4n3/8/4Q1B1/K7 w - - 0 1");
    const chess::Move move = legal_move_by_uci(board, "g2e4");

    REQUIRE(chess::engine::static_exchange_eval(board, move) >= 300);
    REQUIRE(board.hash_key() == board.recompute_hash());
}

TEST_CASE("static exchange evaluation handles en-passant and promotion captures") {
    chess::Board en_passant = chess::board_from_fen("k7/8/8/3pP3/8/8/8/7K w - d6 0 1");
    const chess::Move ep_move = legal_move_by_uci(en_passant, "e5d6");
    REQUIRE(chess::engine::static_exchange_eval(en_passant, ep_move) == 100);
    REQUIRE(en_passant.hash_key() == en_passant.recompute_hash());

    chess::Board promotion = chess::board_from_fen("k6r/6P1/8/8/8/8/8/K7 w - - 0 1");
    const chess::Move promotion_move = legal_move_by_uci(promotion, "g7h8q");
    REQUIRE(chess::engine::static_exchange_eval(promotion, promotion_move) == 1300);
    REQUIRE(promotion.hash_key() == promotion.recompute_hash());
}

TEST_CASE("benchmark suite positions search to legal moves") {
    chess::engine::Searcher searcher;
    searcher.set_hash_size_mb(2);

    for (const chess::engine::BenchmarkPosition& position : chess::engine::benchmark_positions()) {
        INFO(position.id);
        chess::Board board = chess::board_from_fen(position.fen);
        searcher.clear();
        const chess::engine::SearchResult result = searcher.search(board, chess::engine::SearchLimits{2});

        REQUIRE(result.depth == 2);
        REQUIRE(result.nodes > 0);
        REQUIRE(is_legal_best_move(board, result.best_move));
    }
}

TEST_CASE("EPD suite loader parses metadata, depth, and best moves") {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "chess_engine_epd_loader_test.epd";
    {
        std::ofstream output(path);
        output << "# comment lines are ignored\n";
        output << "6k1/6pp/8/2B5/8/8/6PP/5RK1 w - - bm f1f8; id \"mate-in-one-back-rank\"; theme \"mate-in-1\"; acd 2; hmvc 0; fmvn 1; c0 \"Protected rook delivers mate\";\n";
        output << "4k3/7q/8/8/8/8/8/4K2R w - - bm h1h7; id \"hanging-queen\"; theme \"hanging queen\"; acd 1;\n";
    }

    const std::vector<chess::engine::SuitePosition> positions = chess::engine::load_epd_suite(path);
    std::filesystem::remove(path);

    REQUIRE(positions.size() == 2);
    REQUIRE(positions[0].id == "mate-in-one-back-rank");
    REQUIRE(positions[0].theme == "mate-in-1");
    REQUIRE(positions[0].description == "Protected rook delivers mate");
    REQUIRE(positions[0].fen == "6k1/6pp/8/2B5/8/8/6PP/5RK1 w - - 0 1");
    REQUIRE(positions[0].depth == 2);
    REQUIRE(positions[0].expected_best_moves == std::vector<std::string>{"f1f8"});
    REQUIRE(chess::engine::is_expected_best_move(positions[0], "f1f8"));
    REQUIRE_FALSE(chess::engine::is_expected_best_move(positions[0], "c5f8"));

    REQUIRE(positions[1].fen == "4k3/7q/8/8/8/8/8/4K2R w - - 0 1");
    REQUIRE(positions[1].depth == 1);
}

TEST_CASE("tactical suite finds expected best moves") {
    chess::engine::Searcher searcher;
    searcher.set_hash_size_mb(2);

    for (const chess::engine::TacticalPosition& position : chess::engine::tactical_positions()) {
        INFO(position.id);
        chess::Board board = chess::board_from_fen(position.fen);
        searcher.clear();
        const chess::engine::SearchResult result = searcher.search(board, chess::engine::SearchLimits{position.depth});
        const std::string best_move = chess::move_to_uci(result.best_move);

        REQUIRE(result.depth == position.depth);
        REQUIRE(is_legal_best_move(board, result.best_move));
        REQUIRE(chess::engine::is_expected_best_move(position, best_move));
    }
}
