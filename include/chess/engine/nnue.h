#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "chess/core/board.h"

namespace chess::engine::nnue {

constexpr std::uint32_t kFormatVersionV1 = 1;
constexpr std::uint32_t kFormatVersionV2 = 2;
constexpr std::uint32_t kFormatVersionV3 = 3;
constexpr std::uint32_t kFormatVersionV4 = 4;
constexpr std::uint32_t kFormatVersionV5 = 5;
constexpr std::uint32_t kFormatVersionV6 = 6;
constexpr std::uint32_t kFormatVersion = kFormatVersionV4;
constexpr std::uint32_t kHalfKpFeatureCount = 64 * 10 * 64;
constexpr std::uint32_t kHalfKaV2HmLiteKingBuckets = 32;
constexpr std::uint32_t kHalfKaV2HmLiteFeatureCount = kHalfKaV2HmLiteKingBuckets * 10 * 64;
constexpr std::uint32_t kFullThreatsFeatureCount = 60720;
constexpr std::uint32_t kFullThreatsMaxActive = 128;
constexpr std::uint32_t kHalfKaV2HmFullThreatsFeatureCount =
    kHalfKaV2HmLiteFeatureCount + kFullThreatsFeatureCount;
constexpr std::uint32_t kFeatureCount = kHalfKpFeatureCount;
constexpr std::uint32_t kDefaultHiddenSize = 256;
constexpr std::uint32_t kMaxHiddenSize = 1024;
constexpr std::uint32_t kDefaultAccumulatorScale = 256;
constexpr std::uint32_t kDefaultOutputScale = 1024;

enum class FeatureSet : std::uint32_t {
    HalfKp = 1,
    HalfKaV2HmLite = 2,
    HalfKaV2HmFullThreats = 3,
};

struct ModelInfo {
    std::uint32_t format_version = 0;
    std::uint32_t feature_count = 0;
    std::uint32_t hidden_size = 0;
    std::uint32_t accumulator_scale = 0;
    std::uint32_t output_scale = 0;
    bool side_to_move_perspective = false;
    bool sf_lite = false;
    std::uint32_t l2_size = 0;
    std::uint32_t l3_size = 0;
    std::uint32_t placement_feature_count = 0;
    std::uint32_t threat_feature_count = 0;
    FeatureSet feature_set = FeatureSet::HalfKp;
    std::filesystem::path path;
};

struct QuantizedAccumulatorPair {
    std::vector<int> white;
    std::vector<int> black;
    bool valid = false;
};

struct ProfileCounters {
    std::uint64_t refresh_placement_ns = 0;
    std::uint64_t refresh_threat_ns = 0;
    std::uint64_t update_placement_ns = 0;
    std::uint64_t update_threat_ns = 0;
    std::uint64_t dense_convert_ns = 0;
    std::uint64_t dense_forward_ns = 0;
    std::uint64_t dirty_squares = 0;
    std::uint64_t dirty_attackers_before = 0;
    std::uint64_t dirty_attackers_after = 0;
    std::uint64_t dirty_threat_removed = 0;
    std::uint64_t dirty_threat_added = 0;
    std::uint64_t dirty_threat_unchanged = 0;
    std::uint64_t fallback_parent_invalid = 0;
    std::uint64_t fallback_unsupported = 0;
    std::uint64_t fallback_invalid_move = 0;
    std::uint64_t fallback_missing_king = 0;
    std::uint64_t fallback_moving_king = 0;
    std::uint64_t fallback_placement_feature = 0;
    std::uint64_t fallback_dirty_overflow = 0;
    std::uint64_t partial_refreshes = 0;
};

[[nodiscard]] Square perspective_square(Square square, Color perspective);
[[nodiscard]] Square horizontal_mirror_square(Square square);
[[nodiscard]] std::uint32_t feature_count(FeatureSet feature_set);
[[nodiscard]] std::uint32_t feature_index(
    Color perspective,
    Square perspective_king,
    Piece piece,
    Square square,
    FeatureSet feature_set = FeatureSet::HalfKp
);
[[nodiscard]] std::uint32_t full_threat_feature_index(
    Color perspective,
    Square perspective_king,
    Piece attacker,
    Square attacker_square,
    Piece victim,
    Square target_square
);
[[nodiscard]] std::vector<std::uint32_t> active_feature_indices(
    const Board& board,
    Color perspective,
    FeatureSet feature_set = FeatureSet::HalfKp
);

class Network {
public:
    [[nodiscard]] bool load(const std::filesystem::path& path, std::string* error = nullptr);
    void clear();

    [[nodiscard]] bool loaded() const;
    [[nodiscard]] const ModelInfo& info() const;
    [[nodiscard]] int evaluate_white_perspective(const Board& board) const;
    [[nodiscard]] bool supports_quantized_accumulator_stack() const;
    void refresh_quantized_accumulator_pair(
        const Board& board,
        QuantizedAccumulatorPair& pair,
        ProfileCounters* profile = nullptr
    ) const;
    [[nodiscard]] bool update_quantized_accumulator_pair_after_move(
        const Board& board_after_move,
        const UndoState& undo,
        const QuantizedAccumulatorPair& parent,
        QuantizedAccumulatorPair& child,
        ProfileCounters* profile = nullptr
    ) const;
    [[nodiscard]] int evaluate_white_perspective(
        const Board& board,
        const QuantizedAccumulatorPair& accumulator_pair,
        ProfileCounters* profile = nullptr
    ) const;

private:
    ModelInfo info_{};
    std::vector<std::int16_t> feature_bias_;
    std::vector<std::int16_t> feature_weights_;
    std::vector<std::int16_t> output_weights_;
    std::int32_t output_bias_ = 0;
    std::int32_t side_to_move_weight_ = 0;
    std::vector<float> hidden_bias_float_;
    std::vector<float> feature_weights_float_;
    std::vector<float> placement_feature_weights_float_;
    std::vector<float> threat_feature_weights_float_;
    std::vector<std::int16_t> placement_feature_weights_quantized_;
    std::vector<std::int8_t> threat_feature_weights_quantized_;
    std::vector<float> direct_weights_;
    float direct_bias_ = 0.0F;
    std::vector<float> fc0_weights_;
    std::vector<float> fc0_bias_;
    std::vector<float> fc1_weights_;
    std::vector<float> fc1_bias_;
    std::vector<float> fc2_weights_;
    float fc2_bias_ = 0.0F;
    float side_to_move_weight_float_ = 0.0F;

    [[nodiscard]] std::vector<int> accumulator(const Board& board, Color perspective) const;
    [[nodiscard]] std::vector<float> accumulator_float(const Board& board, Color perspective) const;
    [[nodiscard]] std::vector<float> accumulator_float_from_quantized(
        const Board& board,
        Color perspective,
        const std::vector<int>& quantized_accumulator
    ) const;
    [[nodiscard]] int evaluate_sf_lite_white_perspective(const Board& board) const;
    void refresh_quantized_accumulator(
        const Board& board,
        Color perspective,
        std::vector<int>& accumulator,
        ProfileCounters* profile = nullptr
    ) const;
    [[nodiscard]] int evaluate_sf_lite_white_perspective(
        const Board& board,
        const std::vector<float>& white,
        const std::vector<float>& black,
        ProfileCounters* profile = nullptr
    ) const;
    [[nodiscard]] int evaluate_sf_lite_white_perspective_clamped(
        const Board& board,
        const std::vector<float>& white,
        const std::vector<float>& black,
        std::vector<float>& fc0,
        std::vector<float>& fc1,
        ProfileCounters* profile = nullptr
    ) const;
    [[nodiscard]] int evaluate_sf_lite_white_perspective_quantized(
        const Board& board,
        const std::vector<int>& white,
        const std::vector<int>& black,
        ProfileCounters* profile = nullptr
    ) const;
};

}  // namespace chess::engine::nnue
