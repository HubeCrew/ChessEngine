#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "chess/core/board.h"

namespace chess::engine::nnue {

constexpr std::uint32_t kFormatVersionV1 = 1;
constexpr std::uint32_t kFormatVersion = 2;
constexpr std::uint32_t kFeatureCount = 64 * 10 * 64;
constexpr std::uint32_t kDefaultHiddenSize = 256;
constexpr std::uint32_t kMaxHiddenSize = 1024;
constexpr std::uint32_t kDefaultAccumulatorScale = 256;
constexpr std::uint32_t kDefaultOutputScale = 1024;

struct ModelInfo {
    std::uint32_t format_version = 0;
    std::uint32_t feature_count = 0;
    std::uint32_t hidden_size = 0;
    std::uint32_t accumulator_scale = 0;
    std::uint32_t output_scale = 0;
    std::filesystem::path path;
};

[[nodiscard]] Square perspective_square(Square square, Color perspective);
[[nodiscard]] std::uint32_t feature_index(Color perspective, Square perspective_king, Piece piece, Square square);
[[nodiscard]] std::vector<std::uint32_t> active_feature_indices(const Board& board, Color perspective);

class Network {
public:
    [[nodiscard]] bool load(const std::filesystem::path& path, std::string* error = nullptr);
    void clear();

    [[nodiscard]] bool loaded() const;
    [[nodiscard]] const ModelInfo& info() const;
    [[nodiscard]] int evaluate_white_perspective(const Board& board) const;

private:
    ModelInfo info_{};
    std::vector<std::int16_t> feature_bias_;
    std::vector<std::int16_t> feature_weights_;
    std::vector<std::int16_t> output_weights_;
    std::int32_t output_bias_ = 0;
    std::int32_t side_to_move_weight_ = 0;

    [[nodiscard]] std::vector<int> accumulator(const Board& board, Color perspective) const;
};

}  // namespace chess::engine::nnue
