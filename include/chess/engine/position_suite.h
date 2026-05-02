#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace chess::engine {

struct BenchmarkPosition {
    std::string_view id;
    std::string_view description;
    std::string_view fen;
    int depth;
};

struct TacticalPosition {
    std::string_view id;
    std::string_view theme;
    std::string_view description;
    std::string_view fen;
    int depth;
    std::array<std::string_view, 4> expected_best_moves;
    std::size_t expected_best_move_count;
};

struct SuitePosition {
    std::string id;
    std::string theme;
    std::string description;
    std::string fen;
    int depth = 1;
    std::vector<std::string> expected_best_moves;
};

std::span<const BenchmarkPosition> benchmark_positions();
std::span<const TacticalPosition> tactical_positions();
bool is_expected_best_move(const TacticalPosition& position, std::string_view move);
bool is_expected_best_move(const SuitePosition& position, std::string_view move);
std::vector<SuitePosition> load_epd_suite(const std::filesystem::path& path);

}  // namespace chess::engine
