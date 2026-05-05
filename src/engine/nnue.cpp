#include "chess/engine/nnue.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <utility>

#include "chess/core/attacks.h"

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

bool feature_set_from_id(std::uint32_t identifier, FeatureSet& feature_set) {
    switch (identifier) {
        case static_cast<std::uint32_t>(FeatureSet::HalfKp):
            feature_set = FeatureSet::HalfKp;
            return true;
        case static_cast<std::uint32_t>(FeatureSet::HalfKaV2HmLite):
            feature_set = FeatureSet::HalfKaV2HmLite;
            return true;
        case static_cast<std::uint32_t>(FeatureSet::HalfKaV2HmFullThreats):
            feature_set = FeatureSet::HalfKaV2HmFullThreats;
            return true;
        default:
            return false;
    }
}

struct HalfKaV2HmOrientation {
    int king_bucket = 0;
    bool mirror = false;
};

HalfKaV2HmOrientation halfka_v2_hm_orientation(Square perspective_king, Color perspective) {
    Square king = perspective_square(perspective_king, perspective);
    HalfKaV2HmOrientation orientation{};
    orientation.mirror = file_of(king) >= 4;
    if (orientation.mirror) {
        king = horizontal_mirror_square(king);
    }
    orientation.king_bucket = rank_of(king) * 4 + file_of(king);
    return orientation;
}

Square halfka_v2_hm_oriented_square(Square square, Color perspective, bool mirror) {
    Square oriented = perspective_square(square, perspective);
    return mirror ? horizontal_mirror_square(oriented) : oriented;
}

constexpr std::array<int, 6> kFullThreatsValidTargets{{6, 12, 10, 10, 12, 8}};

constexpr std::array<std::array<int, 6>, 6> kFullThreatsTargetMap{{
    {{0, 1, -1, 2, -1, -1}},
    {{0, 1, 2, 3, 4, 5}},
    {{0, 1, 2, 3, -1, 4}},
    {{0, 1, 2, 3, -1, 4}},
    {{0, 1, 2, 3, 4, 5}},
    {{0, 1, 2, 3, -1, -1}},
}};

int full_threat_piece_type_slot(PieceType type) {
    if (type == PieceType::None) {
        return -1;
    }
    return static_cast<int>(type);
}

int full_threat_piece_slot(Piece piece, Color perspective) {
    if (piece == Piece::None || type_of(piece) == PieceType::None) {
        return -1;
    }
    const int relative_color = color_of(piece) == perspective ? 0 : 1;
    return relative_color * 6 + full_threat_piece_type_slot(type_of(piece));
}

Bitboard full_threat_attacks(int attacker_slot, Square square, Bitboard occupancy) {
    const Color relative_color = attacker_slot < 6 ? Color::White : Color::Black;
    const PieceType type = static_cast<PieceType>(attacker_slot % 6);
    return attacks::piece_attacks(type, relative_color, square, occupancy);
}

struct FullThreatTables {
    std::array<std::array<std::uint32_t, kBoardSquareCount>, 12> offsets{};
    std::array<std::uint32_t, 12> attack_counts{};
    std::array<std::uint32_t, 12> piece_offsets{};
    std::array<std::array<std::array<std::uint16_t, kBoardSquareCount>, kBoardSquareCount>, 12> attack_ordinals{};
    std::array<std::array<std::array<std::uint32_t, 2>, 12>, 12> pair_offsets{};
};

FullThreatTables build_full_threat_tables() {
    FullThreatTables tables{};
    for (auto& attacker : tables.pair_offsets) {
        for (auto& victim : attacker) {
            victim.fill(kFullThreatsFeatureCount);
        }
    }

    std::uint32_t cumulative_offset = 0;
    for (int attacker_slot = 0; attacker_slot < 12; ++attacker_slot) {
        const PieceType attacker_type = static_cast<PieceType>(attacker_slot % 6);
        std::uint32_t cumulative_piece_offset = 0;
        for (Square square = 0; square < kBoardSquareCount; ++square) {
            tables.offsets[static_cast<std::size_t>(attacker_slot)][static_cast<std::size_t>(square)] = cumulative_piece_offset;
            const Bitboard attacks = full_threat_attacks(attacker_slot, square, 0);
            for (Square target = 0; target < kBoardSquareCount; ++target) {
                const Bitboard lower_targets = target == 0 ? 0 : ((Bitboard{1} << target) - 1);
                tables.attack_ordinals[static_cast<std::size_t>(attacker_slot)][static_cast<std::size_t>(square)]
                                      [static_cast<std::size_t>(target)] = static_cast<std::uint16_t>(
                                          __builtin_popcountll(attacks & lower_targets)
                                      );
            }
            if (attacker_type != PieceType::Pawn || (rank_of(square) >= 1 && rank_of(square) <= 6)) {
                cumulative_piece_offset += static_cast<std::uint32_t>(__builtin_popcountll(attacks));
            }
        }
        tables.attack_counts[static_cast<std::size_t>(attacker_slot)] = cumulative_piece_offset;
        tables.piece_offsets[static_cast<std::size_t>(attacker_slot)] = cumulative_offset;
        cumulative_offset += static_cast<std::uint32_t>(kFullThreatsValidTargets[static_cast<std::size_t>(attacker_type)])
                           * cumulative_piece_offset;
    }

    for (int attacker_slot = 0; attacker_slot < 12; ++attacker_slot) {
        const int attacker_color = attacker_slot < 6 ? 0 : 1;
        const PieceType attacker_type = static_cast<PieceType>(attacker_slot % 6);
        for (int victim_slot = 0; victim_slot < 12; ++victim_slot) {
            const int victim_color = victim_slot < 6 ? 0 : 1;
            const PieceType victim_type = static_cast<PieceType>(victim_slot % 6);
            const int mapped = kFullThreatsTargetMap[static_cast<std::size_t>(attacker_type)]
                                                    [static_cast<std::size_t>(victim_type)];
            if (mapped < 0) {
                continue;
            }
            const bool enemy = attacker_color != victim_color;
            const bool semi_excluded = attacker_type == victim_type && (enemy || attacker_type != PieceType::Pawn);
            const int target_bucket =
                victim_color * (kFullThreatsValidTargets[static_cast<std::size_t>(attacker_type)] / 2) + mapped;
            const std::uint32_t feature =
                tables.piece_offsets[static_cast<std::size_t>(attacker_slot)]
                + static_cast<std::uint32_t>(target_bucket)
                    * tables.attack_counts[static_cast<std::size_t>(attacker_slot)];
            tables.pair_offsets[static_cast<std::size_t>(attacker_slot)][static_cast<std::size_t>(victim_slot)][0] =
                feature;
            if (!semi_excluded) {
                tables.pair_offsets[static_cast<std::size_t>(attacker_slot)][static_cast<std::size_t>(victim_slot)][1] =
                    feature;
            }
        }
    }

    return tables;
}

const FullThreatTables& full_threat_tables() {
    static const FullThreatTables tables = build_full_threat_tables();
    return tables;
}

}  // namespace

Square perspective_square(Square square, Color perspective) {
    if (perspective == Color::White || !is_valid_square(square)) {
        return square;
    }
    return make_square(file_of(square), 7 - rank_of(square));
}

Square horizontal_mirror_square(Square square) {
    if (!is_valid_square(square)) {
        return square;
    }
    return make_square(7 - file_of(square), rank_of(square));
}

std::uint32_t feature_count(FeatureSet feature_set) {
    switch (feature_set) {
        case FeatureSet::HalfKp:
            return kHalfKpFeatureCount;
        case FeatureSet::HalfKaV2HmLite:
            return kHalfKaV2HmLiteFeatureCount;
        case FeatureSet::HalfKaV2HmFullThreats:
            return kHalfKaV2HmFullThreatsFeatureCount;
    }
    return 0;
}

std::uint32_t feature_index(Color perspective, Square perspective_king, Piece piece, Square square, FeatureSet feature_set) {
    const PieceType type = type_of(piece);
    if (piece == Piece::None || type == PieceType::King || type == PieceType::None) {
        return feature_count(feature_set);
    }

    Square king = perspective_square(perspective_king, perspective);
    Square piece_square = perspective_square(square, perspective);
    if (!is_valid_square(king) || !is_valid_square(piece_square)) {
        return feature_count(feature_set);
    }

    const int relative_color = color_of(piece) == perspective ? 0 : 1;
    const int piece_slot = relative_color * 5 + static_cast<int>(type);
    if (feature_set == FeatureSet::HalfKp) {
        return static_cast<std::uint32_t>(((king * 10 + piece_slot) * 64) + piece_square);
    }
    const HalfKaV2HmOrientation orientation = halfka_v2_hm_orientation(perspective_king, perspective);
    piece_square = halfka_v2_hm_oriented_square(square, perspective, orientation.mirror);
    return static_cast<std::uint32_t>(((orientation.king_bucket * 10 + piece_slot) * 64) + piece_square);
}

std::uint32_t full_threat_feature_index(
    Color perspective,
    Square perspective_king,
    Piece attacker,
    Square attacker_square,
    Piece victim,
    Square target_square
) {
    const int attacker_slot = full_threat_piece_slot(attacker, perspective);
    const int victim_slot = full_threat_piece_slot(victim, perspective);
    if (attacker_slot < 0 || victim_slot < 0 || !is_valid_square(perspective_king)
        || !is_valid_square(attacker_square) || !is_valid_square(target_square)) {
        return kHalfKaV2HmFullThreatsFeatureCount;
    }
    const HalfKaV2HmOrientation orientation = halfka_v2_hm_orientation(perspective_king, perspective);
    const Square from = halfka_v2_hm_oriented_square(attacker_square, perspective, orientation.mirror);
    const Square target = halfka_v2_hm_oriented_square(target_square, perspective, orientation.mirror);
    if (type_of(attacker) == PieceType::Pawn && (rank_of(from) < 1 || rank_of(from) > 6)) {
        return kHalfKaV2HmFullThreatsFeatureCount;
    }
    if ((full_threat_attacks(attacker_slot, from, 0) & square_bb(target)) == 0) {
        return kHalfKaV2HmFullThreatsFeatureCount;
    }
    const auto& tables = full_threat_tables();
    const int direction_slot = from < target ? 1 : 0;
    const std::uint32_t base = tables.pair_offsets[static_cast<std::size_t>(attacker_slot)]
                                                   [static_cast<std::size_t>(victim_slot)]
                                                   [static_cast<std::size_t>(direction_slot)];
    if (base >= kFullThreatsFeatureCount) {
        return kHalfKaV2HmFullThreatsFeatureCount;
    }
    return kHalfKaV2HmLiteFeatureCount + base
         + tables.offsets[static_cast<std::size_t>(attacker_slot)][static_cast<std::size_t>(from)]
         + tables.attack_ordinals[static_cast<std::size_t>(attacker_slot)][static_cast<std::size_t>(from)]
                                 [static_cast<std::size_t>(target)];
}

std::vector<std::uint32_t> active_feature_indices(const Board& board, Color perspective, FeatureSet feature_set) {
    std::vector<std::uint32_t> features;
    features.reserve(feature_set == FeatureSet::HalfKaV2HmFullThreats ? 160 : 30);
    const std::uint32_t maximum_feature = feature_count(feature_set);

    const Square king = board.king_square(perspective);
    if (!is_valid_square(king)) {
        return features;
    }

    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const Piece piece = board.piece_at(square);
        if (piece == Piece::None || type_of(piece) == PieceType::King) {
            continue;
        }
        const std::uint32_t index = feature_index(perspective, king, piece, square, feature_set);
        if (index < maximum_feature) {
            features.push_back(index);
        }
    }
    if (feature_set == FeatureSet::HalfKaV2HmFullThreats) {
        const Bitboard occupied = board.occupancy();
        for (Color color : {Color::White, Color::Black}) {
            for (PieceType type : {PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King}) {
                Bitboard attackers = board.pieces(color, type);
                while (attackers != 0) {
                    const Square from = __builtin_ctzll(attackers);
                    attackers &= attackers - 1;
                    Bitboard targets = attacks::piece_attacks(type, color, from, occupied) & occupied;
                    while (targets != 0) {
                        const Square target = __builtin_ctzll(targets);
                        targets &= targets - 1;
                        const std::uint32_t index = full_threat_feature_index(
                            perspective,
                            king,
                            make_piece(color, type),
                            from,
                            board.piece_at(target),
                            target
                        );
                        if (index < maximum_feature) {
                            features.push_back(index);
                        }
                    }
                }
            }
        }
    }
    std::sort(features.begin(), features.end());
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

    if (version != kFormatVersionV1
        && version != kFormatVersionV2
        && version != kFormatVersionV3
        && version != kFormatVersionV4
        && version != kFormatVersionV5) {
        if (error != nullptr) {
            *error = "unsupported NNUE version";
        }
        return false;
    }
    FeatureSet loaded_feature_set = FeatureSet::HalfKp;
    if (version >= kFormatVersionV5) {
        std::uint32_t feature_set_identifier = 0;
        if (!read_value(input, feature_set_identifier)) {
            if (error != nullptr) {
                *error = "truncated NNUE feature-set header";
            }
            return false;
        }
        if (!feature_set_from_id(feature_set_identifier, loaded_feature_set)) {
            if (error != nullptr) {
                *error = "unsupported NNUE feature set";
            }
            return false;
        }
    }
    if (feature_count != chess::engine::nnue::feature_count(loaded_feature_set) || hidden_size == 0 || hidden_size > kMaxHiddenSize) {
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

    if (version >= kFormatVersionV4) {
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
            loaded_feature_set,
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
        loaded_feature_set,
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

    for (const std::uint32_t feature : active_feature_indices(board, perspective, info_.feature_set)) {
        const std::size_t offset = static_cast<std::size_t>(feature) * info_.hidden_size;
        for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
            values[index] += feature_weights_[offset + index];
        }
    }
    return values;
}

std::vector<float> Network::accumulator_float(const Board& board, Color perspective) const {
    std::vector<float> values = hidden_bias_float_;
    for (const std::uint32_t feature : active_feature_indices(board, perspective, info_.feature_set)) {
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
