#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

#include "chess/core/fen.h"
#include "chess/core/movegen.h"
#include "chess/engine/evaluation.h"
#include "chess/engine/nnue.h"
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

template <typename T>
void write_binary_value(std::ofstream& output, const T& value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

std::filesystem::path write_constant_nnue_model(
    int centipawns,
    std::uint32_t hidden_size = 2,
    std::uint32_t format_version = chess::engine::nnue::kFormatVersionV3,
    int white_to_move_bonus = 0
) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "chess_engine_constant_test.nnue";
    std::ofstream output(path, std::ios::binary);
    const char magic[8] = {'C', 'E', 'N', 'N', 'U', 'E', '1', '\0'};
    output.write(magic, sizeof(magic));
    write_binary_value(output, format_version);
    write_binary_value(output, chess::engine::nnue::kFeatureCount);
    write_binary_value(output, hidden_size);
    write_binary_value(output, chess::engine::nnue::kDefaultAccumulatorScale);
    write_binary_value(output, chess::engine::nnue::kDefaultOutputScale);

    const std::vector<std::int16_t> hidden_bias(hidden_size, 0);
    output.write(reinterpret_cast<const char*>(hidden_bias.data()), static_cast<std::streamsize>(hidden_bias.size() * sizeof(std::int16_t)));

    const std::vector<std::int16_t> feature_weights(
        static_cast<std::size_t>(chess::engine::nnue::kFeatureCount) * hidden_size,
        0
    );
    output.write(
        reinterpret_cast<const char*>(feature_weights.data()),
        static_cast<std::streamsize>(feature_weights.size() * sizeof(std::int16_t))
    );

    const std::vector<std::int16_t> output_weights(static_cast<std::size_t>(hidden_size) * 2, 0);
    output.write(reinterpret_cast<const char*>(output_weights.data()), static_cast<std::streamsize>(output_weights.size() * sizeof(std::int16_t)));

    const std::int32_t output_bias = static_cast<std::int32_t>(centipawns * static_cast<int>(chess::engine::nnue::kDefaultOutputScale));
    write_binary_value(output, output_bias);
    if (format_version >= chess::engine::nnue::kFormatVersionV2) {
        const std::int32_t side_to_move_weight = static_cast<std::int32_t>(
            white_to_move_bonus * static_cast<int>(chess::engine::nnue::kDefaultOutputScale)
        );
        write_binary_value(output, side_to_move_weight);
    }
    return path;
}

std::filesystem::path write_constant_sf_lite_nnue_model(int side_to_move_centipawns, std::uint32_t hidden_size = 2) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "chess_engine_constant_sf_lite_test.nnue";
    std::ofstream output(path, std::ios::binary);
    const char magic[8] = {'C', 'E', 'N', 'N', 'U', 'E', '1', '\0'};
    output.write(magic, sizeof(magic));
    write_binary_value(output, chess::engine::nnue::kFormatVersion);
    write_binary_value(output, chess::engine::nnue::kFeatureCount);
    write_binary_value(output, hidden_size);
    write_binary_value(output, chess::engine::nnue::kDefaultAccumulatorScale);
    write_binary_value(output, chess::engine::nnue::kDefaultOutputScale);

    const std::uint32_t l2_size = 1;
    const std::uint32_t l3_size = 1;
    write_binary_value(output, l2_size);
    write_binary_value(output, l3_size);

    const std::vector<float> hidden_bias(hidden_size, 0.0F);
    output.write(reinterpret_cast<const char*>(hidden_bias.data()), static_cast<std::streamsize>(hidden_bias.size() * sizeof(float)));

    const std::vector<float> feature_weights(
        static_cast<std::size_t>(chess::engine::nnue::kFeatureCount) * hidden_size,
        0.0F
    );
    output.write(reinterpret_cast<const char*>(feature_weights.data()), static_cast<std::streamsize>(feature_weights.size() * sizeof(float)));

    const std::vector<float> direct_weights(static_cast<std::size_t>(hidden_size) * 2, 0.0F);
    output.write(reinterpret_cast<const char*>(direct_weights.data()), static_cast<std::streamsize>(direct_weights.size() * sizeof(float)));
    const float direct_bias = static_cast<float>(side_to_move_centipawns);
    write_binary_value(output, direct_bias);

    const std::vector<float> fc0_weights(static_cast<std::size_t>(l2_size) * hidden_size * 2, 0.0F);
    const std::vector<float> fc0_bias(l2_size, 0.0F);
    const std::vector<float> fc1_weights(static_cast<std::size_t>(l3_size) * l2_size * 2, 0.0F);
    const std::vector<float> fc1_bias(l3_size, 0.0F);
    const std::vector<float> fc2_weights(l3_size, 0.0F);
    output.write(reinterpret_cast<const char*>(fc0_weights.data()), static_cast<std::streamsize>(fc0_weights.size() * sizeof(float)));
    output.write(reinterpret_cast<const char*>(fc0_bias.data()), static_cast<std::streamsize>(fc0_bias.size() * sizeof(float)));
    output.write(reinterpret_cast<const char*>(fc1_weights.data()), static_cast<std::streamsize>(fc1_weights.size() * sizeof(float)));
    output.write(reinterpret_cast<const char*>(fc1_bias.data()), static_cast<std::streamsize>(fc1_bias.size() * sizeof(float)));
    output.write(reinterpret_cast<const char*>(fc2_weights.data()), static_cast<std::streamsize>(fc2_weights.size() * sizeof(float)));
    const float fc2_bias = 0.0F;
    const float side_to_move_weight = 0.0F;
    write_binary_value(output, fc2_bias);
    write_binary_value(output, side_to_move_weight);
    return path;
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

TEST_CASE("evaluation trace totals match public evaluation") {
    const chess::Board board = chess::board_from_fen(
        "r2q1rk1/ppp2ppp/2n2n2/3bp3/3P4/2PBPN2/PP3PPP/RNBQ1RK1 w - - 0 8"
    );
    const chess::engine::EvalTrace trace = chess::engine::evaluate_trace_white_perspective(board);
    const int component_total = trace.material
        + trace.piece_square
        + trace.mobility
        + trace.safe_mobility
        + trace.king_safety
        + trace.threats
        + trace.pawn_structure
        + trace.outposts
        + trace.rook_files
        + trace.space
        + trace.center_control
        + trace.bishop_quality
        + trace.pawn_dynamics
        + trace.development
        + trace.trade_context;

    REQUIRE(trace.total == component_total);
    REQUIRE(trace.total == chess::engine::evaluate_white_perspective(board));
}

TEST_CASE("NNUE feature extraction is deterministic and perspective-aware") {
    const chess::Board board = chess::Board::start_position();
    const std::vector<std::uint32_t> white_once = chess::engine::nnue::active_feature_indices(board, chess::Color::White);
    const std::vector<std::uint32_t> white_twice = chess::engine::nnue::active_feature_indices(board, chess::Color::White);
    const std::vector<std::uint32_t> black_once = chess::engine::nnue::active_feature_indices(board, chess::Color::Black);

    REQUIRE(white_once == white_twice);
    REQUIRE(white_once.size() == 30);
    REQUIRE(black_once.size() == 30);
    REQUIRE(std::all_of(white_once.begin(), white_once.end(), [](std::uint32_t index) {
        return index < chess::engine::nnue::kFeatureCount;
    }));

    const std::uint32_t white_feature = chess::engine::nnue::feature_index(
        chess::Color::White,
        chess::make_square(4, 0),
        chess::Piece::WhiteKnight,
        chess::make_square(2, 2)
    );
    const std::uint32_t mirrored_black_feature = chess::engine::nnue::feature_index(
        chess::Color::Black,
        chess::make_square(4, 7),
        chess::Piece::BlackKnight,
        chess::make_square(2, 5)
    );
    REQUIRE(white_feature == mirrored_black_feature);
}

TEST_CASE("NNUE model loading and opt-in searcher evaluation are safe") {
    const std::filesystem::path path = write_constant_nnue_model(123);
    chess::engine::nnue::Network network;
    std::string error;
    REQUIRE(network.load(path, &error));
    REQUIRE(network.loaded());
    REQUIRE(network.info().hidden_size == 2);

    const chess::Board board = chess::Board::start_position();
    REQUIRE(network.evaluate_white_perspective(board) == 123);
    REQUIRE(std::filesystem::remove(path));

    chess::engine::Searcher searcher;
    const int classical = searcher.evaluate_white_perspective(board);
    searcher.set_eval_type(chess::engine::EvalType::Nnue);
    REQUIRE(searcher.evaluate_white_perspective(board) == classical);

    const std::filesystem::path second_path = write_constant_nnue_model(-75);
    REQUIRE(searcher.load_nnue(second_path, &error));
    REQUIRE(searcher.nnue_loaded());
    REQUIRE(searcher.evaluate_white_perspective(board) == -75);
    searcher.set_eval_type(chess::engine::EvalType::Hybrid);
    REQUIRE(searcher.evaluate_white_perspective(board) == (classical - 75) / 2);
    REQUIRE(std::filesystem::remove(second_path));
}

TEST_CASE("NNUE loader supports legacy models and v4 SF-lite side-to-move perspective") {
    chess::engine::nnue::Network network;
    std::string error;

    const std::filesystem::path v1_path = write_constant_nnue_model(
        64,
        2,
        chess::engine::nnue::kFormatVersionV1
    );
    REQUIRE(network.load(v1_path, &error));
    REQUIRE(network.loaded());
    REQUIRE(network.info().format_version == chess::engine::nnue::kFormatVersionV1);
    REQUIRE(network.evaluate_white_perspective(chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1")) == 64);
    REQUIRE(network.evaluate_white_perspective(chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 b - - 0 1")) == 64);
    REQUIRE(std::filesystem::remove(v1_path));

    const std::filesystem::path v2_path = write_constant_nnue_model(
        100,
        2,
        chess::engine::nnue::kFormatVersionV2,
        12
    );
    REQUIRE(network.load(v2_path, &error));
    REQUIRE(network.loaded());
    REQUIRE(network.info().format_version == chess::engine::nnue::kFormatVersionV2);
    REQUIRE(network.evaluate_white_perspective(chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1")) == 112);
    REQUIRE(network.evaluate_white_perspective(chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 b - - 0 1")) == 88);
    REQUIRE(std::filesystem::remove(v2_path));

    const std::filesystem::path v3_path = write_constant_nnue_model(
        100,
        2,
        chess::engine::nnue::kFormatVersionV3,
        12
    );
    REQUIRE(network.load(v3_path, &error));
    REQUIRE(network.loaded());
    REQUIRE(network.info().format_version == chess::engine::nnue::kFormatVersionV3);
    REQUIRE(network.info().side_to_move_perspective);
    REQUIRE(network.evaluate_white_perspective(chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1")) == 112);
    REQUIRE(network.evaluate_white_perspective(chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 b - - 0 1")) == -112);
    REQUIRE(std::filesystem::remove(v3_path));

    const std::filesystem::path v4_path = write_constant_sf_lite_nnue_model(73);
    REQUIRE(network.load(v4_path, &error));
    REQUIRE(network.loaded());
    REQUIRE(network.info().format_version == chess::engine::nnue::kFormatVersion);
    REQUIRE(network.info().side_to_move_perspective);
    REQUIRE(network.info().sf_lite);
    REQUIRE(network.evaluate_white_perspective(chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1")) == 73);
    REQUIRE(network.evaluate_white_perspective(chess::board_from_fen("4k3/8/8/8/8/8/8/4K3 b - - 0 1")) == -73);
    REQUIRE(std::filesystem::remove(v4_path));
}

TEST_CASE("NNUE loader rejects invalid model files without keeping partial state") {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "chess_engine_invalid_test.nnue";
    {
        std::ofstream output(path, std::ios::binary);
        output << "not-nnue";
    }

    chess::engine::nnue::Network network;
    std::string error;
    REQUIRE_FALSE(network.load(path, &error));
    REQUIRE_FALSE(network.loaded());
    REQUIRE_FALSE(error.empty());
    REQUIRE(std::filesystem::remove(path));

    const std::filesystem::path valid_path = write_constant_nnue_model(42);
    REQUIRE(network.load(valid_path, &error));
    REQUIRE(network.loaded());
    REQUIRE_FALSE(network.load(path, &error));
    REQUIRE(network.loaded());
    REQUIRE(network.evaluate_white_perspective(chess::Board::start_position()) == 42);
    REQUIRE(std::filesystem::remove(valid_path));
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

TEST_CASE("evaluation rewards lower-value pressure on major pieces") {
    const chess::Board queen_attacked_by_knight = chess::board_from_fen("k7/8/8/8/1q6/3N4/8/7K b - - 0 1");
    const chess::Board queen_not_attacked = chess::board_from_fen("k7/8/8/8/1q6/8/3N4/7K b - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(queen_attacked_by_knight)
            > chess::engine::evaluate_white_perspective(queen_not_attacked) + 40);
}

TEST_CASE("evaluation rewards pawn attacks on higher-value pieces") {
    const chess::Board pawn_attacks_rook = chess::board_from_fen("k7/8/8/4r3/3P4/8/8/7K w - - 0 1");
    const chess::Board pawn_misses_rook = chess::board_from_fen("k7/8/8/4r3/2P5/8/8/7K w - - 0 1");

    REQUIRE(chess::engine::evaluate_white_perspective(pawn_attacks_rook)
            > chess::engine::evaluate_white_perspective(pawn_misses_rook));
}

TEST_CASE("evaluation trace rewards safe mobility over pawn-hit mobility") {
    const chess::Board safe = chess::board_from_fen("4k3/8/8/8/3N4/8/8/4K3 w - - 0 1");
    const chess::Board pawn_hit = chess::board_from_fen("4k3/8/2p1p3/8/3N4/8/8/4K3 w - - 0 1");

    REQUIRE(chess::engine::evaluate_trace_white_perspective(safe).safe_mobility
            > chess::engine::evaluate_trace_white_perspective(pawn_hit).safe_mobility);
}

TEST_CASE("evaluation trace exposes strategic positional terms") {
    const chess::Board outpost = chess::board_from_fen("4k3/8/8/3N4/2P1P3/8/8/4K3 w - - 0 1");
    const chess::Board rook_file = chess::board_from_fen("4k3/8/8/8/8/8/8/3RK3 w - - 0 1");
    const chess::Board space = chess::board_from_fen("4k3/8/8/3PP3/8/8/8/4K3 w - - 0 1");
    const chess::Board center = chess::board_from_fen("4k3/8/8/8/3PP3/8/8/4K3 w - - 0 1");

    REQUIRE(chess::engine::evaluate_trace_white_perspective(outpost).outposts > 0);
    REQUIRE(chess::engine::evaluate_trace_white_perspective(rook_file).rook_files > 0);
    REQUIRE(chess::engine::evaluate_trace_white_perspective(space).space > 0);
    REQUIRE(chess::engine::evaluate_trace_white_perspective(center).center_control > 0);
}

TEST_CASE("evaluation trace detects bad bishops and pawn dynamics") {
    const chess::Board bad_bishop = chess::board_from_fen("4k3/8/8/1P1P1P2/P1B1P3/8/8/4K3 w - - 0 1");
    const chess::Board active_bishop = chess::board_from_fen("4k3/8/8/8/8/2B5/8/4K3 w - - 0 1");
    const chess::Board candidate = chess::board_from_fen("4k3/8/8/3P4/2P5/8/8/4K3 w - - 0 1");

    REQUIRE(chess::engine::evaluate_trace_white_perspective(bad_bishop).bishop_quality
            < chess::engine::evaluate_trace_white_perspective(active_bishop).bishop_quality);
    REQUIRE(chess::engine::evaluate_trace_white_perspective(candidate).pawn_dynamics > 0);
}

TEST_CASE("evaluation trace rewards keeping attacking complexity") {
    const chess::Board attacking = chess::board_from_fen("q5k1/5ppp/8/6Q1/2B5/5N2/8/6K1 w - - 0 1");
    const chess::engine::EvalTrace trace = chess::engine::evaluate_trace_white_perspective(attacking);

    REQUIRE(trace.trade_context > 0);
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

TEST_CASE("search can be constrained to explicit legal root moves") {
    chess::Board board = chess::Board::start_position();
    chess::engine::SearchLimits limits;
    limits.depth = 2;
    limits.search_moves.push_back(legal_move_by_uci(board, "e2e4"));

    chess::engine::Searcher searcher;
    const chess::engine::SearchResult result = searcher.search(board, limits);

    REQUIRE(result.depth == 2);
    REQUIRE(chess::move_to_uci(result.best_move) == "e2e4");
    REQUIRE(board.hash_key() == board.recompute_hash());
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
        output << "4k3/7q/8/8/8/8/8/4K2R w - - bm h1h7; am e1d2; id \"hanging-queen\"; theme \"hanging queen\"; acd 1;\n";
        output << "4k3/8/8/8/8/8/8/4K2R w - - am e1d2; id \"avoid-only\"; theme \"postmortem\"; acd 1;\n";
    }

    const std::vector<chess::engine::SuitePosition> positions = chess::engine::load_epd_suite(path);
    std::filesystem::remove(path);

    REQUIRE(positions.size() == 3);
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
    REQUIRE(positions[1].expected_best_moves == std::vector<std::string>{"h1h7"});
    REQUIRE(positions[1].avoided_moves == std::vector<std::string>{"e1d2"});
    REQUIRE(chess::engine::is_expected_best_move(positions[1], "h1h7"));
    REQUIRE(chess::engine::is_avoided_move(positions[1], "e1d2"));
    REQUIRE_FALSE(chess::engine::is_avoided_move(positions[1], "h1h7"));

    REQUIRE(positions[2].expected_best_moves.empty());
    REQUIRE(positions[2].avoided_moves == std::vector<std::string>{"e1d2"});
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
