#include <chrono>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "chess/core/fen.h"
#include "chess/core/move.h"
#include "chess/engine/position_suite.h"
#include "chess/engine/search.h"

namespace {

enum class SuiteSelection {
    Bench,
    Tactics,
    All,
    Epd,
};

struct Options {
    SuiteSelection suite = SuiteSelection::All;
    int depth_override = 0;
    std::size_t hash_mb = 64;
    bool csv = false;
    bool progress = false;
    bool diagnostics = false;
    bool null_move_pruning = true;
    bool search_extensions = true;
    std::vector<std::filesystem::path> epd_paths;
};

struct RunResult {
    std::string id;
    std::string theme;
    std::string description;
    int depth = 0;
    std::string best_move;
    std::string expected;
    std::string avoided;
    bool matched = true;
    int score = 0;
    std::uint64_t nodes = 0;
    std::uint64_t qnodes = 0;
    std::uint64_t nps = 0;
    std::int64_t time_ms = 0;
    chess::engine::SearchDiagnostics diagnostics;
};

struct ProgressState {
    std::size_t completed = 0;
    std::size_t matched = 0;
    std::uint64_t total_nodes = 0;
    std::uint64_t total_qnodes = 0;
    std::int64_t total_time_ms = 0;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
};

void print_usage(std::ostream& out) {
    out << "usage: chess_bench [--suite bench|tactics|all|epd] [--epd PATH] [--depth N] [--hash MB] [--disable-null-move] [--disable-extensions] [--progress] [--diagnostics] [--csv]\n";
}

SuiteSelection parse_suite(std::string_view value) {
    if (value == "bench") {
        return SuiteSelection::Bench;
    }
    if (value == "tactics") {
        return SuiteSelection::Tactics;
    }
    if (value == "all") {
        return SuiteSelection::All;
    }
    if (value == "epd") {
        return SuiteSelection::Epd;
    }
    throw std::invalid_argument("suite must be bench, tactics, all, or epd");
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--help" || arg == "-h") {
            print_usage(std::cout);
            std::exit(0);
        }
        if (arg == "--csv") {
            options.csv = true;
        } else if (arg == "--progress") {
            options.progress = true;
        } else if (arg == "--diagnostics") {
            options.diagnostics = true;
        } else if (arg == "--disable-null-move") {
            options.null_move_pruning = false;
        } else if (arg == "--disable-extensions") {
            options.search_extensions = false;
        } else if (arg == "--epd") {
            if (++index >= argc) {
                throw std::invalid_argument("--epd requires a path");
            }
            options.epd_paths.emplace_back(argv[index]);
        } else if (arg == "--suite") {
            if (++index >= argc) {
                throw std::invalid_argument("--suite requires a value");
            }
            options.suite = parse_suite(argv[index]);
        } else if (arg == "--depth") {
            if (++index >= argc) {
                throw std::invalid_argument("--depth requires a value");
            }
            options.depth_override = std::stoi(argv[index]);
            if (options.depth_override <= 0) {
                throw std::invalid_argument("--depth must be positive");
            }
        } else if (arg == "--hash") {
            if (++index >= argc) {
                throw std::invalid_argument("--hash requires a value");
            }
            const int hash = std::stoi(argv[index]);
            if (hash <= 0) {
                throw std::invalid_argument("--hash must be positive");
            }
            options.hash_mb = static_cast<std::size_t>(hash);
        } else {
            throw std::invalid_argument("unknown option: " + arg);
        }
    }
    return options;
}

std::string expected_moves_string(const chess::engine::SuitePosition& position) {
    std::string result;
    for (const std::string& move : position.expected_best_moves) {
        if (!result.empty()) {
            result.push_back('|');
        }
        result += move;
    }
    return result;
}

std::string avoided_moves_string(const chess::engine::SuitePosition& position) {
    std::string result;
    for (const std::string& move : position.avoided_moves) {
        if (!result.empty()) {
            result.push_back('|');
        }
        result += move;
    }
    return result;
}

std::string expected_moves_string(const chess::engine::TacticalPosition& position) {
    std::string result;
    for (std::size_t index = 0; index < position.expected_best_move_count; ++index) {
        if (!result.empty()) {
            result.push_back('|');
        }
        result += position.expected_best_moves[index];
    }
    return result;
}

RunResult run_search(
    chess::engine::Searcher& searcher,
    std::string_view id,
    std::string_view theme,
    std::string_view description,
    std::string_view fen,
    int depth,
    std::string expected = {}
) {
    chess::Board board = chess::board_from_fen(fen);
    const chess::engine::SearchResult search_result = searcher.search(board, chess::engine::SearchLimits{depth});

    RunResult result;
    result.id = id;
    result.theme = theme;
    result.description = description;
    result.depth = search_result.depth;
    result.best_move = chess::move_to_uci(search_result.best_move);
    result.expected = std::move(expected);
    result.score = search_result.score_centipawns;
    result.nodes = search_result.nodes;
    result.qnodes = search_result.qnodes;
    result.nps = search_result.nps;
    result.time_ms = search_result.elapsed.count();
    result.diagnostics = search_result.diagnostics;
    return result;
}

void print_csv_header() {
    std::cout << "id,theme,depth,bestmove,expected,avoided,matched,score_cp,nodes,nps,time_ms,description,qnodes\n";
}

std::string csv_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (const char ch : value) {
        if (ch == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

void print_csv_row(const RunResult& result) {
    std::cout << csv_escape(result.id) << ','
              << csv_escape(result.theme) << ','
              << result.depth << ','
              << csv_escape(result.best_move) << ','
              << csv_escape(result.expected) << ','
              << csv_escape(result.avoided) << ','
              << (result.matched ? "true" : "false") << ','
              << result.score << ','
              << result.nodes << ','
              << result.nps << ','
              << result.time_ms << ','
              << csv_escape(result.description) << ','
              << result.qnodes << '\n';
}

void print_table_header() {
    std::cout << std::left
              << std::setw(24) << "id"
              << std::setw(16) << "theme"
              << std::right
              << std::setw(6) << "depth"
              << std::setw(10) << "best"
              << std::setw(14) << "expected"
              << std::setw(8) << "ok"
              << std::setw(10) << "score"
              << std::setw(14) << "nodes"
              << std::setw(14) << "qnodes"
              << std::setw(14) << "nps"
              << std::setw(10) << "ms"
              << '\n';
}

void print_table_row(const RunResult& result) {
    std::cout << std::left
              << std::setw(24) << result.id
              << std::setw(16) << result.theme
              << std::right
              << std::setw(6) << result.depth
              << std::setw(10) << result.best_move
              << std::setw(14) << (result.expected.empty() ? "-" : result.expected)
              << std::setw(8) << (result.matched ? "yes" : "NO")
              << std::setw(10) << result.score
              << std::setw(14) << result.nodes
              << std::setw(14) << result.qnodes
              << std::setw(14) << result.nps
              << std::setw(10) << result.time_ms
              << '\n';
}

void print_progress_update(const RunResult& result, std::size_t total, ProgressState& progress) {
    ++progress.completed;
    if (result.matched) {
        ++progress.matched;
    }
    progress.total_nodes += result.nodes;
    progress.total_qnodes += result.qnodes;
    progress.total_time_ms += result.time_ms;

    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - progress.start
    ).count();
    const std::size_t missed = progress.completed - progress.matched;

    std::cerr << '[' << progress.completed << '/' << total << "] "
              << result.id
              << " theme=" << result.theme
              << " depth=" << result.depth
              << " best=" << result.best_move;
    if (!result.expected.empty()) {
        std::cerr << " expected=" << result.expected;
    }
    std::cerr << " ok=" << (result.matched ? "yes" : "NO")
              << " score=" << result.score
              << " nodes=" << result.nodes
              << " qnodes=" << result.qnodes
              << " time_ms=" << result.time_ms
              << " matched=" << progress.matched
              << " missed=" << missed
              << " elapsed_s=" << elapsed
              << '\n';
}

chess::engine::SearchDiagnostics add_diagnostics(
    chess::engine::SearchDiagnostics lhs,
    const chess::engine::SearchDiagnostics& rhs
) {
    lhs.evaluations += rhs.evaluations;
    lhs.move_picker_pv_picks += rhs.move_picker_pv_picks;
    lhs.move_picker_tt_picks += rhs.move_picker_tt_picks;
    lhs.move_picker_scored_moves += rhs.move_picker_scored_moves;
    lhs.move_picker_searched_moves += rhs.move_picker_searched_moves;
    lhs.move_picker_tactical_picks += rhs.move_picker_tactical_picks;
    lhs.move_picker_killer_picks += rhs.move_picker_killer_picks;
    lhs.move_picker_quiet_picks += rhs.move_picker_quiet_picks;
    lhs.beta_cutoffs += rhs.beta_cutoffs;
    lhs.beta_cutoff_move_index_sum += rhs.beta_cutoff_move_index_sum;
    lhs.illegal_pseudo_moves += rhs.illegal_pseudo_moves;
    lhs.null_move_attempts += rhs.null_move_attempts;
    lhs.null_move_cutoffs += rhs.null_move_cutoffs;
    lhs.lmr_reductions += rhs.lmr_reductions;
    lhs.lmr_researches += rhs.lmr_researches;
    lhs.qsearch_in_check_nodes += rhs.qsearch_in_check_nodes;
    lhs.qsearch_stand_pat_nodes += rhs.qsearch_stand_pat_nodes;
    lhs.see_calls += rhs.see_calls;
    lhs.move_gives_check_calls += rhs.move_gives_check_calls;
    return lhs;
}

void print_diagnostics_summary(const chess::engine::SearchDiagnostics& diagnostics, std::ostream& out) {
    const auto average_cutoff_index = diagnostics.beta_cutoffs == 0
        ? 0.0
        : static_cast<double>(diagnostics.beta_cutoff_move_index_sum) / static_cast<double>(diagnostics.beta_cutoffs);

    out << "diagnostics\n"
        << "  evaluations " << diagnostics.evaluations << '\n'
        << "  move_picker_pv_picks " << diagnostics.move_picker_pv_picks << '\n'
        << "  move_picker_tt_picks " << diagnostics.move_picker_tt_picks << '\n'
        << "  move_picker_scored_moves " << diagnostics.move_picker_scored_moves << '\n'
        << "  move_picker_searched_moves " << diagnostics.move_picker_searched_moves << '\n'
        << "  move_picker_tactical_picks " << diagnostics.move_picker_tactical_picks << '\n'
        << "  move_picker_killer_picks " << diagnostics.move_picker_killer_picks << '\n'
        << "  move_picker_quiet_picks " << diagnostics.move_picker_quiet_picks << '\n'
        << "  beta_cutoffs " << diagnostics.beta_cutoffs << '\n'
        << "  beta_cutoff_avg_move_index " << std::fixed << std::setprecision(2) << average_cutoff_index << std::defaultfloat << '\n'
        << "  illegal_pseudo_moves " << diagnostics.illegal_pseudo_moves << '\n'
        << "  null_move_attempts " << diagnostics.null_move_attempts << '\n'
        << "  null_move_cutoffs " << diagnostics.null_move_cutoffs << '\n'
        << "  lmr_reductions " << diagnostics.lmr_reductions << '\n'
        << "  lmr_researches " << diagnostics.lmr_researches << '\n'
        << "  qsearch_in_check_nodes " << diagnostics.qsearch_in_check_nodes << '\n'
        << "  qsearch_stand_pat_nodes " << diagnostics.qsearch_stand_pat_nodes << '\n'
        << "  see_calls " << diagnostics.see_calls << '\n'
        << "  move_gives_check_calls " << diagnostics.move_gives_check_calls << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        if (options.suite == SuiteSelection::Epd && options.epd_paths.empty()) {
            throw std::invalid_argument("--suite epd requires at least one --epd PATH");
        }
        chess::engine::Searcher searcher;
        searcher.set_hash_size_mb(options.hash_mb);
        searcher.set_null_move_pruning(options.null_move_pruning);
        searcher.set_search_extensions(options.search_extensions);

        std::vector<RunResult> results;
        std::size_t total_positions = 0;
        if (options.suite == SuiteSelection::Bench || options.suite == SuiteSelection::All) {
            total_positions += chess::engine::benchmark_positions().size();
        }
        if (options.suite == SuiteSelection::Tactics || options.suite == SuiteSelection::All) {
            total_positions += chess::engine::tactical_positions().size();
        }

        std::vector<std::vector<chess::engine::SuitePosition>> epd_suites;
        epd_suites.reserve(options.epd_paths.size());
        for (const std::filesystem::path& path : options.epd_paths) {
            epd_suites.push_back(chess::engine::load_epd_suite(path));
            total_positions += epd_suites.back().size();
        }

        ProgressState progress;
        if (options.progress) {
            std::cerr << "progress start positions=" << total_positions << '\n';
        }

        if (options.suite == SuiteSelection::Bench || options.suite == SuiteSelection::All) {
            for (const chess::engine::BenchmarkPosition& position : chess::engine::benchmark_positions()) {
                searcher.clear();
                const int depth = options.depth_override > 0 ? options.depth_override : position.depth;
                RunResult result = run_search(searcher, position.id, "bench", position.description, position.fen, depth);
                if (options.progress) {
                    print_progress_update(result, total_positions, progress);
                }
                results.push_back(std::move(result));
            }
        }

        if (options.suite == SuiteSelection::Tactics || options.suite == SuiteSelection::All) {
            for (const chess::engine::TacticalPosition& position : chess::engine::tactical_positions()) {
                searcher.clear();
                const int depth = options.depth_override > 0 ? options.depth_override : position.depth;
                RunResult result = run_search(
                    searcher,
                    position.id,
                    position.theme,
                    position.description,
                    position.fen,
                    depth,
                    expected_moves_string(position)
                );
                result.matched = chess::engine::is_expected_best_move(position, result.best_move);
                if (options.progress) {
                    print_progress_update(result, total_positions, progress);
                }
                results.push_back(std::move(result));
            }
        }

        for (const std::vector<chess::engine::SuitePosition>& suite : epd_suites) {
            for (const chess::engine::SuitePosition& position : suite) {
                searcher.clear();
                const int depth = options.depth_override > 0 ? options.depth_override : position.depth;
                RunResult result = run_search(
                    searcher,
                    position.id,
                    position.theme,
                    position.description,
                    position.fen,
                    depth,
                    expected_moves_string(position)
                );
                result.avoided = avoided_moves_string(position);
                if (!position.expected_best_moves.empty() || !position.avoided_moves.empty()) {
                    const bool expected_ok = position.expected_best_moves.empty()
                        || chess::engine::is_expected_best_move(position, result.best_move);
                    const bool avoided_ok = !chess::engine::is_avoided_move(position, result.best_move);
                    result.matched = expected_ok && avoided_ok;
                }
                if (options.progress) {
                    print_progress_update(result, total_positions, progress);
                }
                results.push_back(std::move(result));
            }
        }

        bool all_matched = true;
        std::uint64_t total_nodes = 0;
        std::uint64_t total_qnodes = 0;
        std::int64_t total_time_ms = 0;
        chess::engine::SearchDiagnostics total_diagnostics;
        for (const RunResult& result : results) {
            all_matched = all_matched && result.matched;
            total_nodes += result.nodes;
            total_qnodes += result.qnodes;
            total_time_ms += result.time_ms;
            total_diagnostics = add_diagnostics(total_diagnostics, result.diagnostics);
        }

        if (options.csv) {
            print_csv_header();
            for (const RunResult& result : results) {
                print_csv_row(result);
            }
        } else {
            print_table_header();
            for (const RunResult& result : results) {
                print_table_row(result);
            }
            const auto total_nps = total_nodes * 1000ULL / static_cast<std::uint64_t>(std::max<std::int64_t>(1, total_time_ms));
            std::cout << "\npositions " << results.size()
                      << " total_nodes " << total_nodes
                      << " total_qnodes " << total_qnodes
                      << " total_time_ms " << total_time_ms
                      << " total_nps " << total_nps << '\n';
            if (options.diagnostics) {
                std::cout << '\n';
                print_diagnostics_summary(total_diagnostics, std::cout);
            }
        }

        if (options.csv && options.diagnostics) {
            print_diagnostics_summary(total_diagnostics, std::cerr);
        }

        return all_matched ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        print_usage(std::cerr);
        return 2;
    }
}
