#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

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

std::span<const BenchmarkPosition> benchmark_positions();
std::span<const TacticalPosition> tactical_positions();
bool is_expected_best_move(const TacticalPosition& position, std::string_view move);

}  // namespace chess::engine

