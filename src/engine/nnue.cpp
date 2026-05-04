#include "chess/engine/nnue.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <utility>

namespace chess::engine::nnue {

namespace {

constexpr std::array<char, 8> kMagic{{'C', 'E', 'N', 'N', 'U', 'E', '1', '\0'}};

template <typename T>
bool read_value(std::istream& input, T& value) {
    input.read(reinterpret_cast<char*>(&value), sizeof(T));
    return static_cast<bool>(input);
}

template <typename T>
bool read_vector(std::istream& input, std::vector<T>& values, std::size_t count) {
    values.resize(count);
    if (count == 0) {
        return true;
    }
    input.read(reinterpret_cast<char*>(values.data()), static_cast<std::streamsize>(sizeof(T) * count));
    return static_cast<bool>(input);
}

std::uint32_t checked_weight_count(std::uint32_t feature_count, std::uint32_t hidden_size) {
    const std::uint64_t count = static_cast<std::uint64_t>(feature_count) * static_cast<std::uint64_t>(hidden_size);
    if (count > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
        return 0;
    }
    return static_cast<std::uint32_t>(count);
}

int rounded_divide(std::int64_t value, std::int64_t scale) {
    if (scale <= 0) {
        return 0;
    }
    if (value >= 0) {
        return static_cast<int>((value + scale / 2) / scale);
    }
    return static_cast<int>((value - scale / 2) / scale);
}

}  // namespace

Square perspective_square(Square square, Color perspective) {
    if (perspective == Color::White || !is_valid_square(square)) {
        return square;
    }
    return make_square(file_of(square), 7 - rank_of(square));
}

std::uint32_t feature_index(Color perspective, Square perspective_king, Piece piece, Square square) {
    const PieceType type = type_of(piece);
    if (piece == Piece::None || type == PieceType::King || type == PieceType::None) {
        return kFeatureCount;
    }

    const Square king = perspective_square(perspective_king, perspective);
    const Square piece_square = perspective_square(square, perspective);
    if (!is_valid_square(king) || !is_valid_square(piece_square)) {
        return kFeatureCount;
    }

    const int relative_color = color_of(piece) == perspective ? 0 : 1;
    const int piece_slot = relative_color * 5 + static_cast<int>(type);
    return static_cast<std::uint32_t>(((king * 10 + piece_slot) * 64) + piece_square);
}

std::vector<std::uint32_t> active_feature_indices(const Board& board, Color perspective) {
    std::vector<std::uint32_t> features;
    features.reserve(30);

    const Square king = board.king_square(perspective);
    if (!is_valid_square(king)) {
        return features;
    }

    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const Piece piece = board.piece_at(square);
        if (piece == Piece::None || type_of(piece) == PieceType::King) {
            continue;
        }
        const std::uint32_t index = feature_index(perspective, king, piece, square);
        if (index < kFeatureCount) {
            features.push_back(index);
        }
    }
    return features;
}

bool Network::load(const std::filesystem::path& path, std::string* error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        if (error != nullptr) {
            *error = "could not open NNUE file: " + path.string();
        }
        return false;
    }

    std::array<char, 8> magic{};
    input.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!input || magic != kMagic) {
        if (error != nullptr) {
            *error = "invalid NNUE magic";
        }
        return false;
    }

    std::uint32_t version = 0;
    std::uint32_t feature_count = 0;
    std::uint32_t hidden_size = 0;
    std::uint32_t accumulator_scale = 0;
    std::uint32_t output_scale = 0;
    if (!read_value(input, version)
        || !read_value(input, feature_count)
        || !read_value(input, hidden_size)
        || !read_value(input, accumulator_scale)
        || !read_value(input, output_scale)) {
        if (error != nullptr) {
            *error = "truncated NNUE header";
        }
        return false;
    }

    if (version != kFormatVersionV1 && version != kFormatVersionV2 && version != kFormatVersionV3 && version != kFormatVersion) {
        if (error != nullptr) {
            *error = "unsupported NNUE version";
        }
        return false;
    }
    if (feature_count != kFeatureCount || hidden_size == 0 || hidden_size > kMaxHiddenSize) {
        if (error != nullptr) {
            *error = "unsupported NNUE shape";
        }
        return false;
    }
    if (accumulator_scale == 0 || output_scale == 0) {
        if (error != nullptr) {
            *error = "invalid NNUE scale";
        }
        return false;
    }

    const std::uint32_t feature_weight_count = checked_weight_count(feature_count, hidden_size);
    if (feature_weight_count == 0) {
        if (error != nullptr) {
            *error = "NNUE weight table is too large";
        }
        return false;
    }

    if (version >= kFormatVersion) {
        std::uint32_t l2_size = 0;
        std::uint32_t l3_size = 0;
        if (!read_value(input, l2_size) || !read_value(input, l3_size)) {
            if (error != nullptr) {
                *error = "truncated SF-lite NNUE dense header";
            }
            return false;
        }
        if (l2_size == 0 || l2_size > 512 || l3_size == 0 || l3_size > 512) {
            if (error != nullptr) {
                *error = "unsupported SF-lite NNUE dense shape";
            }
            return false;
        }

        std::vector<float> hidden_bias;
        std::vector<float> feature_weights;
        std::vector<float> direct_weights;
        float direct_bias = 0.0F;
        std::vector<float> fc0_weights;
        std::vector<float> fc0_bias;
        std::vector<float> fc1_weights;
        std::vector<float> fc1_bias;
        std::vector<float> fc2_weights;
        float fc2_bias = 0.0F;
        float side_to_move_weight = 0.0F;
        if (!read_vector(input, hidden_bias, hidden_size)
            || !read_vector(input, feature_weights, feature_weight_count)
            || !read_vector(input, direct_weights, static_cast<std::size_t>(hidden_size) * 2)
            || !read_value(input, direct_bias)
            || !read_vector(input, fc0_weights, static_cast<std::size_t>(l2_size) * hidden_size * 2)
            || !read_vector(input, fc0_bias, l2_size)
            || !read_vector(input, fc1_weights, static_cast<std::size_t>(l3_size) * l2_size * 2)
            || !read_vector(input, fc1_bias, l3_size)
            || !read_vector(input, fc2_weights, l3_size)
            || !read_value(input, fc2_bias)
            || !read_value(input, side_to_move_weight)) {
            if (error != nullptr) {
                *error = "truncated SF-lite NNUE weights";
            }
            return false;
        }

        info_ = ModelInfo{
            version,
            feature_count,
            hidden_size,
            accumulator_scale,
            output_scale,
            true,
            true,
            l2_size,
            l3_size,
            path,
        };
        feature_bias_.clear();
        feature_weights_.clear();
        output_weights_.clear();
        output_bias_ = 0;
        side_to_move_weight_ = 0;
        hidden_bias_float_ = std::move(hidden_bias);
        feature_weights_float_ = std::move(feature_weights);
        direct_weights_ = std::move(direct_weights);
        direct_bias_ = direct_bias;
        fc0_weights_ = std::move(fc0_weights);
        fc0_bias_ = std::move(fc0_bias);
        fc1_weights_ = std::move(fc1_weights);
        fc1_bias_ = std::move(fc1_bias);
        fc2_weights_ = std::move(fc2_weights);
        fc2_bias_ = fc2_bias;
        side_to_move_weight_float_ = side_to_move_weight;
        return true;
    }

    std::vector<std::int16_t> feature_bias;
    std::vector<std::int16_t> feature_weights;
    std::vector<std::int16_t> output_weights;
    std::int32_t output_bias = 0;
    std::int32_t side_to_move_weight = 0;
    if (!read_vector(input, feature_bias, hidden_size)
        || !read_vector(input, feature_weights, feature_weight_count)
        || !read_vector(input, output_weights, static_cast<std::size_t>(hidden_size) * 2)
        || !read_value(input, output_bias)) {
        if (error != nullptr) {
            *error = "truncated NNUE weights";
        }
        return false;
    }
    if (version >= kFormatVersionV2 && !read_value(input, side_to_move_weight)) {
        if (error != nullptr) {
            *error = "truncated NNUE side-to-move weight";
        }
        return false;
    }

    info_ = ModelInfo{
        version,
        feature_count,
        hidden_size,
        accumulator_scale,
        output_scale,
        version >= kFormatVersionV3,
        false,
        0,
        0,
        path,
    };
    feature_bias_ = std::move(feature_bias);
    feature_weights_ = std::move(feature_weights);
    output_weights_ = std::move(output_weights);
    output_bias_ = output_bias;
    side_to_move_weight_ = side_to_move_weight;
    hidden_bias_float_.clear();
    feature_weights_float_.clear();
    direct_weights_.clear();
    direct_bias_ = 0.0F;
    fc0_weights_.clear();
    fc0_bias_.clear();
    fc1_weights_.clear();
    fc1_bias_.clear();
    fc2_weights_.clear();
    fc2_bias_ = 0.0F;
    side_to_move_weight_float_ = 0.0F;
    return true;
}

void Network::clear() {
    info_ = {};
    feature_bias_.clear();
    feature_weights_.clear();
    output_weights_.clear();
    output_bias_ = 0;
    side_to_move_weight_ = 0;
    hidden_bias_float_.clear();
    feature_weights_float_.clear();
    direct_weights_.clear();
    direct_bias_ = 0.0F;
    fc0_weights_.clear();
    fc0_bias_.clear();
    fc1_weights_.clear();
    fc1_bias_.clear();
    fc2_weights_.clear();
    fc2_bias_ = 0.0F;
    side_to_move_weight_float_ = 0.0F;
}

bool Network::loaded() const {
    return info_.hidden_size != 0;
}

const ModelInfo& Network::info() const {
    return info_;
}

std::vector<int> Network::accumulator(const Board& board, Color perspective) const {
    std::vector<int> values;
    values.reserve(info_.hidden_size);
    for (const std::int16_t bias : feature_bias_) {
        values.push_back(bias);
    }

    for (const std::uint32_t feature : active_feature_indices(board, perspective)) {
        const std::size_t offset = static_cast<std::size_t>(feature) * info_.hidden_size;
        for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
            values[index] += feature_weights_[offset + index];
        }
    }
    return values;
}

std::vector<float> Network::accumulator_float(const Board& board, Color perspective) const {
    std::vector<float> values = hidden_bias_float_;
    for (const std::uint32_t feature : active_feature_indices(board, perspective)) {
        const std::size_t offset = static_cast<std::size_t>(feature) * info_.hidden_size;
        for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
            values[index] += feature_weights_float_[offset + index];
        }
    }
    return values;
}

int Network::evaluate_sf_lite_white_perspective(const Board& board) const {
    std::vector<float> white = accumulator_float(board, Color::White);
    std::vector<float> black = accumulator_float(board, Color::Black);
    for (float& value : white) {
        value = std::clamp(value, 0.0F, 1.0F);
    }
    for (float& value : black) {
        value = std::clamp(value, 0.0F, 1.0F);
    }

    const bool white_to_move = board.side_to_move() == Color::White;
    const std::vector<float>& stm = white_to_move ? white : black;
    const std::vector<float>& nstm = white_to_move ? black : white;

    float direct = direct_bias_;
    for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
        direct += stm[index] * direct_weights_[index];
        direct += nstm[index] * direct_weights_[info_.hidden_size + index];
    }

    std::vector<float> fc0(info_.l2_size, 0.0F);
    for (std::uint32_t row = 0; row < info_.l2_size; ++row) {
        float value = fc0_bias_[row];
        const std::size_t row_offset = static_cast<std::size_t>(row) * info_.hidden_size * 2;
        for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
            value += stm[index] * fc0_weights_[row_offset + index];
            value += nstm[index] * fc0_weights_[row_offset + info_.hidden_size + index];
        }
        fc0[row] = std::clamp(value, 0.0F, 1.0F);
    }

    std::vector<float> fc1(info_.l3_size, 0.0F);
    for (std::uint32_t row = 0; row < info_.l3_size; ++row) {
        float value = fc1_bias_[row];
        const std::size_t row_offset = static_cast<std::size_t>(row) * info_.l2_size * 2;
        for (std::uint32_t index = 0; index < info_.l2_size; ++index) {
            value += fc0[index] * fc0[index] * fc1_weights_[row_offset + index];
            value += fc0[index] * fc1_weights_[row_offset + info_.l2_size + index];
        }
        fc1[row] = std::clamp(value, 0.0F, 1.0F);
    }

    float side_to_move_score = direct + fc2_bias_ + side_to_move_weight_float_;
    for (std::uint32_t index = 0; index < info_.l3_size; ++index) {
        side_to_move_score += fc1[index] * fc2_weights_[index];
    }
    const int rounded = static_cast<int>(std::lround(side_to_move_score));
    return white_to_move ? rounded : -rounded;
}

int Network::evaluate_white_perspective(const Board& board) const {
    if (!loaded()) {
        return 0;
    }
    if (info_.sf_lite) {
        return evaluate_sf_lite_white_perspective(board);
    }

    const std::vector<int> white = accumulator(board, Color::White);
    const std::vector<int> black = accumulator(board, Color::Black);

    if (info_.side_to_move_perspective) {
        const bool white_to_move = board.side_to_move() == Color::White;
        const std::vector<int>& stm = white_to_move ? white : black;
        const std::vector<int>& nstm = white_to_move ? black : white;
        std::int64_t sum = output_bias_ + static_cast<std::int64_t>(side_to_move_weight_);
        for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
            const int stm_activation = std::clamp(stm[index], 0, static_cast<int>(info_.accumulator_scale));
            const int nstm_activation = std::clamp(nstm[index], 0, static_cast<int>(info_.accumulator_scale));
            sum += static_cast<std::int64_t>(stm_activation) * output_weights_[index];
            sum += static_cast<std::int64_t>(nstm_activation) * output_weights_[info_.hidden_size + index];
        }
        const int side_to_move_score = rounded_divide(sum, info_.output_scale);
        return white_to_move ? side_to_move_score : -side_to_move_score;
    }

    std::int64_t sum = output_bias_;
    sum += (board.side_to_move() == Color::White ? 1 : -1) * static_cast<std::int64_t>(side_to_move_weight_);
    for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
        const int white_activation = std::clamp(white[index], 0, static_cast<int>(info_.accumulator_scale));
        const int black_activation = std::clamp(black[index], 0, static_cast<int>(info_.accumulator_scale));
        sum += static_cast<std::int64_t>(white_activation) * output_weights_[index];
        sum += static_cast<std::int64_t>(black_activation) * output_weights_[info_.hidden_size + index];
    }

    return rounded_divide(sum, info_.output_scale);
}

}  // namespace chess::engine::nnue
