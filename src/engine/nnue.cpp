#include "chess/engine/nnue.h"

#include <algorithm>
#include <array>
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

    if (version != kFormatVersion) {
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

    std::vector<std::int16_t> feature_bias;
    std::vector<std::int16_t> feature_weights;
    std::vector<std::int16_t> output_weights;
    std::int32_t output_bias = 0;
    if (!read_vector(input, feature_bias, hidden_size)
        || !read_vector(input, feature_weights, feature_weight_count)
        || !read_vector(input, output_weights, static_cast<std::size_t>(hidden_size) * 2)
        || !read_value(input, output_bias)) {
        if (error != nullptr) {
            *error = "truncated NNUE weights";
        }
        return false;
    }

    info_ = ModelInfo{
        feature_count,
        hidden_size,
        accumulator_scale,
        output_scale,
        path,
    };
    feature_bias_ = std::move(feature_bias);
    feature_weights_ = std::move(feature_weights);
    output_weights_ = std::move(output_weights);
    output_bias_ = output_bias;
    return true;
}

void Network::clear() {
    info_ = {};
    feature_bias_.clear();
    feature_weights_.clear();
    output_weights_.clear();
    output_bias_ = 0;
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

int Network::evaluate_white_perspective(const Board& board) const {
    if (!loaded()) {
        return 0;
    }

    const std::vector<int> white = accumulator(board, Color::White);
    const std::vector<int> black = accumulator(board, Color::Black);

    std::int64_t sum = output_bias_;
    for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
        const int white_activation = std::clamp(white[index], 0, static_cast<int>(info_.accumulator_scale));
        const int black_activation = std::clamp(black[index], 0, static_cast<int>(info_.accumulator_scale));
        sum += static_cast<std::int64_t>(white_activation) * output_weights_[index];
        sum += static_cast<std::int64_t>(black_activation) * output_weights_[info_.hidden_size + index];
    }

    return rounded_divide(sum, info_.output_scale);
}

}  // namespace chess::engine::nnue
