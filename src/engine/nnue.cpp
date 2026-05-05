#include "chess/engine/nnue.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <utility>

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#endif

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

using ProfileClock = std::chrono::steady_clock;

std::uint64_t elapsed_ns_since(ProfileClock::time_point start) {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(ProfileClock::now() - start).count()
    );
}

template <typename T, std::size_t Capacity>
class SmallList {
public:
    [[nodiscard]] bool push(T value) {
        if (size_ >= Capacity) {
            overflow_ = true;
            return false;
        }
        values_[size_++] = value;
        return true;
    }

    [[nodiscard]] bool push_unique(T value) {
        if (contains(value)) {
            return true;
        }
        return push(value);
    }

    [[nodiscard]] bool contains(T value) const {
        return std::find(begin(), end(), value) != end();
    }

    void sort_unique() {
        std::sort(begin(), end());
        auto unique_end = std::unique(begin(), end());
        size_ = static_cast<std::size_t>(unique_end - begin());
    }

    [[nodiscard]] T* begin() {
        return values_.data();
    }

    [[nodiscard]] T* end() {
        return values_.data() + size_;
    }

    [[nodiscard]] const T* begin() const {
        return values_.data();
    }

    [[nodiscard]] const T* end() const {
        return values_.data() + size_;
    }

    [[nodiscard]] const T& operator[](std::size_t index) const {
        return values_[index];
    }

    [[nodiscard]] std::size_t size() const {
        return size_;
    }

    [[nodiscard]] bool overflow() const {
        return overflow_;
    }

private:
    std::array<T, Capacity> values_{};
    std::size_t size_ = 0;
    bool overflow_ = false;
};

using DirtySquareList = SmallList<Square, 8>;
using DirtyAttackerList = SmallList<Square, 64>;
using DirtyFeatureList = SmallList<std::uint32_t, 2048>;

void add_quantized_feature_scalar(
    std::vector<int>& accumulator,
    const std::vector<std::int16_t>& weights,
    std::uint32_t feature,
    std::uint32_t hidden_size,
    int sign
) {
    const std::size_t offset = static_cast<std::size_t>(feature) * hidden_size;
    for (std::uint32_t index = 0; index < hidden_size; ++index) {
        accumulator[index] += sign * static_cast<int>(weights[offset + index]);
    }
}

void add_quantized_threat_feature_scalar(
    std::vector<int>& accumulator,
    const std::vector<std::int8_t>& weights,
    std::uint32_t feature,
    std::uint32_t hidden_size,
    int sign
) {
    const std::size_t offset = static_cast<std::size_t>(feature) * hidden_size;
    for (std::uint32_t index = 0; index < hidden_size; ++index) {
        accumulator[index] += sign * static_cast<int>(weights[offset + index]);
    }
}

float dot_product_float_scalar(const float* lhs, const float* rhs, std::uint32_t count) {
    float total = 0.0F;
    for (std::uint32_t index = 0; index < count; ++index) {
        total += lhs[index] * rhs[index];
    }
    return total;
}

float dot_product_square_float_scalar(const float* lhs, const float* rhs, std::uint32_t count) {
    float total = 0.0F;
    for (std::uint32_t index = 0; index < count; ++index) {
        total += lhs[index] * lhs[index] * rhs[index];
    }
    return total;
}

void clamp_quantized_to_float_scalar(
    const std::vector<int>& input,
    std::vector<float>& output,
    std::uint32_t hidden_size,
    std::uint32_t accumulator_scale
) {
    output.resize(hidden_size);
    const float inverse_scale = 1.0F / static_cast<float>(accumulator_scale);
    const int scale = static_cast<int>(accumulator_scale);
    for (std::uint32_t index = 0; index < hidden_size; ++index) {
        output[index] = static_cast<float>(std::clamp(input[index], 0, scale)) * inverse_scale;
    }
}

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
[[gnu::target("avx2")]]
void add_quantized_feature_avx2(
    std::vector<int>& accumulator,
    const std::vector<std::int16_t>& weights,
    std::uint32_t feature,
    std::uint32_t hidden_size,
    int sign
) {
    const std::size_t offset = static_cast<std::size_t>(feature) * hidden_size;
    std::uint32_t index = 0;
    const __m256i sign_vector = _mm256_set1_epi32(sign);
    for (; index + 16 <= hidden_size; index += 16) {
        const __m256i current_lo = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(accumulator.data() + index));
        const __m256i current_hi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(accumulator.data() + index + 8));
        const __m128i packed = _mm_loadu_si128(reinterpret_cast<const __m128i*>(weights.data() + offset + index));
        const __m256i weights_lo = _mm256_mullo_epi32(_mm256_cvtepi16_epi32(packed), sign_vector);
        const __m256i weights_hi = _mm256_mullo_epi32(_mm256_cvtepi16_epi32(_mm_srli_si128(packed, 8)), sign_vector);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(accumulator.data() + index), _mm256_add_epi32(current_lo, weights_lo));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(accumulator.data() + index + 8), _mm256_add_epi32(current_hi, weights_hi));
    }
    for (; index < hidden_size; ++index) {
        accumulator[index] += sign * static_cast<int>(weights[offset + index]);
    }
}

[[gnu::target("avx2")]]
void add_quantized_threat_feature_avx2(
    std::vector<int>& accumulator,
    const std::vector<std::int8_t>& weights,
    std::uint32_t feature,
    std::uint32_t hidden_size,
    int sign
) {
    const std::size_t offset = static_cast<std::size_t>(feature) * hidden_size;
    std::uint32_t index = 0;
    const __m256i sign_vector = _mm256_set1_epi32(sign);
    for (; index + 16 <= hidden_size; index += 16) {
        const __m256i current_lo = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(accumulator.data() + index));
        const __m256i current_hi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(accumulator.data() + index + 8));
        const __m128i packed = _mm_loadu_si128(reinterpret_cast<const __m128i*>(weights.data() + offset + index));
        const __m256i weights_lo = _mm256_mullo_epi32(_mm256_cvtepi8_epi32(packed), sign_vector);
        const __m256i weights_hi = _mm256_mullo_epi32(_mm256_cvtepi8_epi32(_mm_srli_si128(packed, 8)), sign_vector);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(accumulator.data() + index), _mm256_add_epi32(current_lo, weights_lo));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(accumulator.data() + index + 8), _mm256_add_epi32(current_hi, weights_hi));
    }
    for (; index < hidden_size; ++index) {
        accumulator[index] += sign * static_cast<int>(weights[offset + index]);
    }
}

[[gnu::target("avx2")]]
float dot_product_float_avx2(const float* lhs, const float* rhs, std::uint32_t count) {
    __m256 sum = _mm256_setzero_ps();
    std::uint32_t index = 0;
    for (; index + 8 <= count; index += 8) {
        const __m256 left = _mm256_loadu_ps(lhs + index);
        const __m256 right = _mm256_loadu_ps(rhs + index);
        sum = _mm256_add_ps(sum, _mm256_mul_ps(left, right));
    }
    alignas(32) float lanes[8];
    _mm256_store_ps(lanes, sum);
    float total = lanes[0] + lanes[1] + lanes[2] + lanes[3] + lanes[4] + lanes[5] + lanes[6] + lanes[7];
    for (; index < count; ++index) {
        total += lhs[index] * rhs[index];
    }
    return total;
}

[[gnu::target("avx2")]]
float dot_product_square_float_avx2(const float* lhs, const float* rhs, std::uint32_t count) {
    __m256 sum = _mm256_setzero_ps();
    std::uint32_t index = 0;
    for (; index + 8 <= count; index += 8) {
        const __m256 left = _mm256_loadu_ps(lhs + index);
        const __m256 right = _mm256_loadu_ps(rhs + index);
        sum = _mm256_add_ps(sum, _mm256_mul_ps(_mm256_mul_ps(left, left), right));
    }
    alignas(32) float lanes[8];
    _mm256_store_ps(lanes, sum);
    float total = lanes[0] + lanes[1] + lanes[2] + lanes[3] + lanes[4] + lanes[5] + lanes[6] + lanes[7];
    for (; index < count; ++index) {
        total += lhs[index] * lhs[index] * rhs[index];
    }
    return total;
}

[[gnu::target("avx2")]]
void clamp_quantized_to_float_avx2(
    const std::vector<int>& input,
    std::vector<float>& output,
    std::uint32_t hidden_size,
    std::uint32_t accumulator_scale
) {
    output.resize(hidden_size);
    const __m256i zero = _mm256_setzero_si256();
    const __m256i scale_i = _mm256_set1_epi32(static_cast<int>(accumulator_scale));
    const __m256 inverse_scale = _mm256_set1_ps(1.0F / static_cast<float>(accumulator_scale));
    std::uint32_t index = 0;
    for (; index + 8 <= hidden_size; index += 8) {
        __m256i value = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input.data() + index));
        value = _mm256_min_epi32(_mm256_max_epi32(value, zero), scale_i);
        const __m256 scaled = _mm256_mul_ps(_mm256_cvtepi32_ps(value), inverse_scale);
        _mm256_storeu_ps(output.data() + index, scaled);
    }
    const float inverse = 1.0F / static_cast<float>(accumulator_scale);
    const int scale = static_cast<int>(accumulator_scale);
    for (; index < hidden_size; ++index) {
        output[index] = static_cast<float>(std::clamp(input[index], 0, scale)) * inverse;
    }
}
#endif

bool cpu_supports_avx2() {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    static const bool supported = [] {
        __builtin_cpu_init();
        return __builtin_cpu_supports("avx2");
    }();
    return supported;
#else
    return false;
#endif
}

void add_quantized_feature(
    std::vector<int>& accumulator,
    const std::vector<std::int16_t>& weights,
    std::uint32_t feature,
    std::uint32_t hidden_size,
    int sign
) {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    if (cpu_supports_avx2()) {
        add_quantized_feature_avx2(accumulator, weights, feature, hidden_size, sign);
        return;
    }
#endif
    add_quantized_feature_scalar(accumulator, weights, feature, hidden_size, sign);
}

void add_quantized_threat_feature(
    std::vector<int>& accumulator,
    const std::vector<std::int8_t>& weights,
    std::uint32_t feature,
    std::uint32_t hidden_size,
    int sign
) {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    if (cpu_supports_avx2()) {
        add_quantized_threat_feature_avx2(accumulator, weights, feature, hidden_size, sign);
        return;
    }
#endif
    add_quantized_threat_feature_scalar(accumulator, weights, feature, hidden_size, sign);
}

float dot_product_float(const float* lhs, const float* rhs, std::uint32_t count) {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    if (cpu_supports_avx2()) {
        return dot_product_float_avx2(lhs, rhs, count);
    }
#endif
    return dot_product_float_scalar(lhs, rhs, count);
}

float dot_product_square_float(const float* lhs, const float* rhs, std::uint32_t count) {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    if (cpu_supports_avx2()) {
        return dot_product_square_float_avx2(lhs, rhs, count);
    }
#endif
    return dot_product_square_float_scalar(lhs, rhs, count);
}

void clamp_quantized_to_float(
    const std::vector<int>& input,
    std::vector<float>& output,
    std::uint32_t hidden_size,
    std::uint32_t accumulator_scale
) {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    if (cpu_supports_avx2()) {
        clamp_quantized_to_float_avx2(input, output, hidden_size, accumulator_scale);
        return;
    }
#endif
    clamp_quantized_to_float_scalar(input, output, hidden_size, accumulator_scale);
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

bool is_slider_for_direction(PieceType type, int df, int dr) {
    if (type == PieceType::Queen) {
        return true;
    }
    if (df != 0 && dr != 0) {
        return type == PieceType::Bishop;
    }
    return type == PieceType::Rook;
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

constexpr std::array<int, 6> kFullThreatsValidTargets{{6, 10, 8, 8, 10, 0}};

constexpr std::array<std::array<int, 6>, 6> kFullThreatsTargetMap{{
    {{0, 1, -1, 2, -1, -1}},
    {{0, 1, 2, 3, 4, -1}},
    {{0, 1, 2, 3, -1, -1}},
    {{0, 1, 2, 3, -1, -1}},
    {{0, 1, 2, 3, 4, -1}},
    {{-1, -1, -1, -1, -1, -1}},
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
    if (type == PieceType::Pawn) {
        Bitboard attacks = attacks::pawn_attacks(relative_color, square);
        const int direction = relative_color == Color::White ? 8 : -8;
        const Square push = square + direction;
        if (is_valid_square(push)) {
            attacks |= square_bb(push);
        }
        return attacks;
    }
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

void push_unique_square(DirtySquareList& squares, Square square) {
    if (!is_valid_square(square)) {
        return;
    }
    (void) squares.push_unique(square);
}

void push_unique_attacker(DirtyAttackerList& attackers, Square square) {
    if (!is_valid_square(square)) {
        return;
    }
    (void) attackers.push_unique(square);
}

DirtySquareList dirty_squares_for_move(const UndoState& undo) {
    DirtySquareList squares;
    const Move& move = undo.move;
    push_unique_square(squares, move.from);
    push_unique_square(squares, move.to);
    if ((move.flags & EnPassant) != 0) {
        const Square captured_square = undo.side_to_move == Color::White ? move.to - 8 : move.to + 8;
        push_unique_square(squares, captured_square);
    }
    if ((move.flags & KingCastle) != 0 || (move.flags & QueenCastle) != 0) {
        const int rank = undo.side_to_move == Color::White ? 0 : 7;
        push_unique_square(squares, (move.flags & KingCastle) != 0 ? make_square(7, rank) : make_square(0, rank));
        push_unique_square(squares, (move.flags & KingCastle) != 0 ? make_square(5, rank) : make_square(3, rank));
    }
    return squares;
}

void collect_slider_attackers_through_dirty_square(const Board& board, Square dirty, DirtyAttackerList& attackers) {
    constexpr std::array<std::pair<int, int>, 8> directions{{
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1},
    }};
    for (const auto& [df, dr] : directions) {
        int file = file_of(dirty) + df;
        int rank = rank_of(dirty) + dr;
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const Square square = make_square(file, rank);
            const Piece piece = board.piece_at(square);
            if (piece != Piece::None) {
                if (is_slider_for_direction(type_of(piece), df, dr)) {
                    push_unique_attacker(attackers, square);
                }
                break;
            }
            file += df;
            rank += dr;
        }
    }
}

void collect_pawn_pushers_to_dirty_square(const Board& board, Square dirty, DirtyAttackerList& attackers) {
    const Square white_from = dirty - 8;
    if (is_valid_square(white_from) && board.piece_at(white_from) == Piece::WhitePawn) {
        push_unique_attacker(attackers, white_from);
    }
    const Square black_from = dirty + 8;
    if (is_valid_square(black_from) && board.piece_at(black_from) == Piece::BlackPawn) {
        push_unique_attacker(attackers, black_from);
    }
}

DirtyAttackerList dirty_threat_attackers(const Board& board, const DirtySquareList& dirty_squares) {
    DirtyAttackerList attackers;
    for (const Square dirty : dirty_squares) {
        const Piece dirty_piece = board.piece_at(dirty);
        if (dirty_piece != Piece::None) {
            push_unique_attacker(attackers, dirty);
        }
        for (Color color : {Color::White, Color::Black}) {
            Bitboard direct_attackers = attacks::attackers_to(board, dirty, color);
            while (direct_attackers != 0) {
                const Square square = __builtin_ctzll(direct_attackers);
                direct_attackers &= direct_attackers - 1;
                push_unique_attacker(attackers, square);
            }
        }
        collect_pawn_pushers_to_dirty_square(board, dirty, attackers);
        collect_slider_attackers_through_dirty_square(board, dirty, attackers);
    }
    return attackers;
}

void append_threat_features_for_attacker(
    const Board& board,
    Color perspective,
    Square perspective_king,
    Square attacker_square,
    DirtyFeatureList& features
) {
    const Piece attacker = board.piece_at(attacker_square);
    if (attacker == Piece::None) {
        return;
    }
    const PieceType type = type_of(attacker);
    if (type == PieceType::King || type == PieceType::None) {
        return;
    }

    Bitboard targets = 0;
    const Color color = color_of(attacker);
    if (type == PieceType::Pawn) {
        targets = attacks::pawn_attacks(color, attacker_square) & board.occupancy();
        const Square push = attacker_square + (color == Color::White ? 8 : -8);
        if (is_valid_square(push)) {
            const Piece pushed = board.piece_at(push);
            if (pushed == Piece::WhitePawn || pushed == Piece::BlackPawn) {
                targets |= square_bb(push);
            }
        }
    } else {
        targets = attacks::piece_attacks(type, color, attacker_square, board.occupancy()) & board.occupancy();
    }

    while (targets != 0) {
        const Square target = __builtin_ctzll(targets);
        targets &= targets - 1;
        const std::uint32_t feature = full_threat_feature_index(
            perspective,
            perspective_king,
            attacker,
            attacker_square,
            board.piece_at(target),
            target
        );
        if (feature >= kHalfKaV2HmLiteFeatureCount && feature < kHalfKaV2HmFullThreatsFeatureCount) {
            (void) features.push(feature - kHalfKaV2HmLiteFeatureCount);
        }
    }
}

DirtyFeatureList dirty_threat_features(
    const Board& board,
    Color perspective,
    const DirtyAttackerList& attackers,
    std::size_t* attacker_count = nullptr
) {
    DirtyFeatureList features;
    const Square king = board.king_square(perspective);
    if (!is_valid_square(king)) {
        return features;
    }
    if (attacker_count != nullptr) {
        *attacker_count += attackers.size();
    }
    if (attackers.overflow()) {
        (void) features.push(kFullThreatsFeatureCount);
        return features;
    }
    for (const Square attacker : attackers) {
        append_threat_features_for_attacker(board, perspective, king, attacker, features);
    }
    features.sort_unique();
    return features;
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
        const Bitboard pawns = board.pieces(Color::White, PieceType::Pawn) | board.pieces(Color::Black, PieceType::Pawn);
        for (Color color : {Color::White, Color::Black}) {
            Bitboard pawn_attackers = board.pieces(color, PieceType::Pawn);
            while (pawn_attackers != 0) {
                const Square from = __builtin_ctzll(pawn_attackers);
                pawn_attackers &= pawn_attackers - 1;
                Bitboard targets = attacks::pawn_attacks(color, from) & occupied;
                const Square push = from + (color == Color::White ? 8 : -8);
                if (is_valid_square(push) && (pawns & square_bb(push)) != 0) {
                    targets |= square_bb(push);
                }
                while (targets != 0) {
                    const Square target = __builtin_ctzll(targets);
                    targets &= targets - 1;
                    const std::uint32_t index = full_threat_feature_index(
                        perspective,
                        king,
                        make_piece(color, PieceType::Pawn),
                        from,
                        board.piece_at(target),
                        target
                    );
                    if (index < maximum_feature) {
                        features.push_back(index);
                    }
                }
            }
            for (PieceType type : {PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen}) {
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
        && version != kFormatVersionV5
        && version != kFormatVersionV6) {
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

    std::uint32_t placement_feature_count = feature_count;
    std::uint32_t threat_feature_count = 0;
    if (version >= kFormatVersionV6) {
        if (!read_value(input, placement_feature_count) || !read_value(input, threat_feature_count)) {
            if (error != nullptr) {
                *error = "truncated NNUE feature-block header";
            }
            return false;
        }
        if (loaded_feature_set != FeatureSet::HalfKaV2HmFullThreats
            || placement_feature_count != kHalfKaV2HmLiteFeatureCount
            || threat_feature_count != kFullThreatsFeatureCount) {
            if (error != nullptr) {
                *error = "unsupported NNUE feature block shape";
            }
            return false;
        }
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
        std::vector<float> placement_feature_weights;
        std::vector<float> threat_feature_weights;
        std::vector<std::int16_t> hidden_bias_quantized;
        std::vector<std::int16_t> placement_feature_weights_quantized;
        std::vector<std::int8_t> threat_feature_weights_quantized;
        std::vector<float> direct_weights;
        float direct_bias = 0.0F;
        std::vector<float> fc0_weights;
        std::vector<float> fc0_bias;
        std::vector<float> fc1_weights;
        std::vector<float> fc1_bias;
        std::vector<float> fc2_weights;
        float fc2_bias = 0.0F;
        float side_to_move_weight = 0.0F;
        bool ok = true;
        if (version >= kFormatVersionV6) {
            ok = read_vector(input, hidden_bias_quantized, hidden_size)
              && read_vector(input, placement_feature_weights_quantized, static_cast<std::size_t>(placement_feature_count) * hidden_size)
              && read_vector(input, threat_feature_weights_quantized, static_cast<std::size_t>(threat_feature_count) * hidden_size);
        } else if (ok) {
            ok = read_vector(input, hidden_bias, hidden_size)
              && read_vector(input, feature_weights, feature_weight_count);
        }
        if (!ok
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
            placement_feature_count,
            threat_feature_count,
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
        placement_feature_weights_float_ = std::move(placement_feature_weights);
        threat_feature_weights_float_ = std::move(threat_feature_weights);
        feature_bias_ = std::move(hidden_bias_quantized);
        placement_feature_weights_quantized_ = std::move(placement_feature_weights_quantized);
        threat_feature_weights_quantized_ = std::move(threat_feature_weights_quantized);
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
        placement_feature_count,
        threat_feature_count,
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
    placement_feature_weights_float_.clear();
    threat_feature_weights_float_.clear();
    placement_feature_weights_quantized_.clear();
    threat_feature_weights_quantized_.clear();
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
    placement_feature_weights_float_.clear();
    threat_feature_weights_float_.clear();
    placement_feature_weights_quantized_.clear();
    threat_feature_weights_quantized_.clear();
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

bool Network::supports_quantized_accumulator_stack() const {
    return loaded()
        && info_.sf_lite
        && info_.feature_set == FeatureSet::HalfKaV2HmFullThreats
        && !feature_bias_.empty()
        && !placement_feature_weights_quantized_.empty()
        && !threat_feature_weights_quantized_.empty();
}

void Network::refresh_quantized_accumulator(
    const Board& board,
    Color perspective,
    std::vector<int>& accumulator,
    ProfileCounters* profile
) const {
    accumulator.assign(feature_bias_.begin(), feature_bias_.end());
    const Square king = board.king_square(perspective);
    if (!is_valid_square(king) || !supports_quantized_accumulator_stack()) {
        return;
    }

    const auto placement_start = profile != nullptr ? ProfileClock::now() : ProfileClock::time_point{};
    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const Piece piece = board.piece_at(square);
        if (piece == Piece::None || type_of(piece) == PieceType::King) {
            continue;
        }
        const std::uint32_t feature = feature_index(
            perspective,
            king,
            piece,
            square,
            FeatureSet::HalfKaV2HmFullThreats
        );
        if (feature < info_.placement_feature_count) {
            add_quantized_feature(accumulator, placement_feature_weights_quantized_, feature, info_.hidden_size, 1);
        }
    }
    if (profile != nullptr) {
        profile->refresh_placement_ns += elapsed_ns_since(placement_start);
    }

    const auto threat_start = profile != nullptr ? ProfileClock::now() : ProfileClock::time_point{};
    for (const std::uint32_t feature : active_feature_indices(board, perspective, FeatureSet::HalfKaV2HmFullThreats)) {
        if (feature >= info_.placement_feature_count && feature < info_.feature_count) {
            add_quantized_threat_feature(
                accumulator,
                threat_feature_weights_quantized_,
                feature - info_.placement_feature_count,
                info_.hidden_size,
                1
            );
        }
    }
    if (profile != nullptr) {
        profile->refresh_threat_ns += elapsed_ns_since(threat_start);
    }
}

void Network::refresh_quantized_accumulator_pair(
    const Board& board,
    QuantizedAccumulatorPair& pair,
    ProfileCounters* profile
) const {
    pair.white.assign(feature_bias_.begin(), feature_bias_.end());
    pair.black.assign(feature_bias_.begin(), feature_bias_.end());
    const Square white_king = board.king_square(Color::White);
    const Square black_king = board.king_square(Color::Black);
    if (!is_valid_square(white_king) || !is_valid_square(black_king) || !supports_quantized_accumulator_stack()) {
        pair.valid = false;
        return;
    }

    const auto placement_start = profile != nullptr ? ProfileClock::now() : ProfileClock::time_point{};
    for (Square square = 0; square < kBoardSquareCount; ++square) {
        const Piece piece = board.piece_at(square);
        if (piece == Piece::None || type_of(piece) == PieceType::King) {
            continue;
        }
        const std::uint32_t white_feature = feature_index(
            Color::White,
            white_king,
            piece,
            square,
            FeatureSet::HalfKaV2HmFullThreats
        );
        const std::uint32_t black_feature = feature_index(
            Color::Black,
            black_king,
            piece,
            square,
            FeatureSet::HalfKaV2HmFullThreats
        );
        if (white_feature < info_.placement_feature_count) {
            add_quantized_feature(pair.white, placement_feature_weights_quantized_, white_feature, info_.hidden_size, 1);
        }
        if (black_feature < info_.placement_feature_count) {
            add_quantized_feature(pair.black, placement_feature_weights_quantized_, black_feature, info_.hidden_size, 1);
        }
    }
    if (profile != nullptr) {
        profile->refresh_placement_ns += elapsed_ns_since(placement_start);
    }

    const auto threat_start = profile != nullptr ? ProfileClock::now() : ProfileClock::time_point{};
    for (const std::uint32_t feature : active_feature_indices(board, Color::White, FeatureSet::HalfKaV2HmFullThreats)) {
        if (feature >= info_.placement_feature_count && feature < info_.feature_count) {
            add_quantized_threat_feature(
                pair.white,
                threat_feature_weights_quantized_,
                feature - info_.placement_feature_count,
                info_.hidden_size,
                1
            );
        }
    }
    for (const std::uint32_t feature : active_feature_indices(board, Color::Black, FeatureSet::HalfKaV2HmFullThreats)) {
        if (feature >= info_.placement_feature_count && feature < info_.feature_count) {
            add_quantized_threat_feature(
                pair.black,
                threat_feature_weights_quantized_,
                feature - info_.placement_feature_count,
                info_.hidden_size,
                1
            );
        }
    }
    if (profile != nullptr) {
        profile->refresh_threat_ns += elapsed_ns_since(threat_start);
    }
    pair.valid = true;
}

bool Network::update_quantized_accumulator_pair_after_move(
    const Board& board_after_move,
    const UndoState& undo,
    const QuantizedAccumulatorPair& parent,
    QuantizedAccumulatorPair& child,
    ProfileCounters* profile
) const {
    if (!parent.valid) {
        if (profile != nullptr) {
            ++profile->fallback_parent_invalid;
        }
        return false;
    }
    if (!supports_quantized_accumulator_stack()) {
        if (profile != nullptr) {
            ++profile->fallback_unsupported;
        }
        return false;
    }
    if (!is_valid_square(undo.move.from) || !is_valid_square(undo.move.to)) {
        if (profile != nullptr) {
            ++profile->fallback_invalid_move;
        }
        return false;
    }

    child = parent;
    const Move& move = undo.move;
    const Color moving_color = undo.side_to_move;
    const Piece moved_before = make_piece(moving_color, (move.flags & Promotion) != 0 ? PieceType::Pawn : type_of(board_after_move.piece_at(move.to)));
    const Piece moved_after = (move.flags & Promotion) != 0
        ? make_piece(moving_color, move.promotion)
        : moved_before;
    const Square captured_square = (move.flags & EnPassant) != 0
        ? (moving_color == Color::White ? move.to - 8 : move.to + 8)
        : move.to;

    auto apply_piece_delta = [&](Color perspective, std::vector<int>& accumulator, Piece piece, Square square, int sign) -> bool {
        if (piece == Piece::None || type_of(piece) == PieceType::King) {
            return true;
        }
        const Square king = board_after_move.king_square(perspective);
        if (!is_valid_square(king)) {
            if (profile != nullptr) {
                ++profile->fallback_missing_king;
            }
            return false;
        }
        const std::uint32_t feature = feature_index(perspective, king, piece, square, FeatureSet::HalfKaV2HmFullThreats);
        if (feature >= info_.placement_feature_count) {
            if (profile != nullptr) {
                ++profile->fallback_placement_feature;
            }
            return false;
        }
        add_quantized_feature(accumulator, placement_feature_weights_quantized_, feature, info_.hidden_size, sign);
        return true;
    };

    bool white_refreshed = false;
    bool black_refreshed = false;
    auto update_for_perspective = [&](Color perspective, std::vector<int>& accumulator) -> bool {
        if (type_of(moved_before) == PieceType::King && perspective == moving_color) {
            if (profile != nullptr) {
                ++profile->partial_refreshes;
            }
            refresh_quantized_accumulator(board_after_move, perspective, accumulator, profile);
            if (perspective == Color::White) {
                white_refreshed = true;
            } else {
                black_refreshed = true;
            }
            return !accumulator.empty();
        }
        if (!apply_piece_delta(perspective, accumulator, moved_before, move.from, -1)) {
            return false;
        }
        if (!apply_piece_delta(perspective, accumulator, undo.captured, captured_square, -1)) {
            return false;
        }
        if (!apply_piece_delta(perspective, accumulator, moved_after, move.to, 1)) {
            return false;
        }

        if ((move.flags & KingCastle) != 0 || (move.flags & QueenCastle) != 0) {
            const int rank = moving_color == Color::White ? 0 : 7;
            const Square rook_from = (move.flags & KingCastle) != 0 ? make_square(7, rank) : make_square(0, rank);
            const Square rook_to = (move.flags & KingCastle) != 0 ? make_square(5, rank) : make_square(3, rank);
            const Piece rook = make_piece(moving_color, PieceType::Rook);
            if (!apply_piece_delta(perspective, accumulator, rook, rook_from, -1)) {
                return false;
            }
            if (!apply_piece_delta(perspective, accumulator, rook, rook_to, 1)) {
                return false;
            }
        }
        return true;
    };

    const auto placement_start = profile != nullptr ? ProfileClock::now() : ProfileClock::time_point{};
    child.valid = update_for_perspective(Color::White, child.white)
               && update_for_perspective(Color::Black, child.black);
    if (profile != nullptr) {
        profile->update_placement_ns += elapsed_ns_since(placement_start);
    }
    if (!child.valid) {
        return false;
    }

    Board board_before_move = board_after_move;
    board_before_move.unmake_move(undo);
    const DirtySquareList dirty_squares = dirty_squares_for_move(undo);
    if (profile != nullptr) {
        profile->dirty_squares += dirty_squares.size();
    }
    if (dirty_squares.overflow()) {
        if (profile != nullptr) {
            ++profile->fallback_dirty_overflow;
        }
        return false;
    }

    const auto threat_start = profile != nullptr ? ProfileClock::now() : ProfileClock::time_point{};
    const DirtyAttackerList before_attackers = dirty_threat_attackers(board_before_move, dirty_squares);
    const DirtyAttackerList after_attackers = dirty_threat_attackers(board_after_move, dirty_squares);
    if (profile != nullptr) {
        profile->dirty_attackers_before += before_attackers.size();
        profile->dirty_attackers_after += after_attackers.size();
    }
    if (before_attackers.overflow() || after_attackers.overflow()) {
        if (profile != nullptr) {
            ++profile->fallback_dirty_overflow;
            profile->update_threat_ns += elapsed_ns_since(threat_start);
        }
        return false;
    }

    auto update_threats_for_perspective = [&](Color perspective, std::vector<int>& accumulator) {
        DirtyFeatureList before = dirty_threat_features(board_before_move, perspective, before_attackers);
        DirtyFeatureList after = dirty_threat_features(board_after_move, perspective, after_attackers);
        if (before.overflow() || after.overflow()) {
            if (profile != nullptr) {
                ++profile->fallback_dirty_overflow;
            }
            return false;
        }
        std::size_t before_index = 0;
        std::size_t after_index = 0;
        while (before_index < before.size() || after_index < after.size()) {
            const std::uint32_t removed = before_index < before.size() ? before[before_index] : kFullThreatsFeatureCount;
            const std::uint32_t added = after_index < after.size() ? after[after_index] : kFullThreatsFeatureCount;
            if (removed >= kFullThreatsFeatureCount && added >= kFullThreatsFeatureCount) {
                return true;
            }
            if (removed == added) {
                if (profile != nullptr) {
                    ++profile->dirty_threat_unchanged;
                }
                ++before_index;
                ++after_index;
            } else if (removed < added) {
                add_quantized_threat_feature(accumulator, threat_feature_weights_quantized_, removed, info_.hidden_size, -1);
                if (profile != nullptr) {
                    ++profile->dirty_threat_removed;
                }
                ++before_index;
            } else {
                add_quantized_threat_feature(accumulator, threat_feature_weights_quantized_, added, info_.hidden_size, 1);
                if (profile != nullptr) {
                    ++profile->dirty_threat_added;
                }
                ++after_index;
            }
        }
        return true;
    };
    child.valid = (white_refreshed || update_threats_for_perspective(Color::White, child.white))
               && (black_refreshed || update_threats_for_perspective(Color::Black, child.black));
    if (profile != nullptr) {
        profile->update_threat_ns += elapsed_ns_since(threat_start);
    }
    return child.valid;
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
    if (info_.feature_set == FeatureSet::HalfKaV2HmFullThreats && !placement_feature_weights_quantized_.empty()) {
        std::vector<int> quantized;
        quantized.reserve(info_.hidden_size);
        for (const std::int16_t bias : feature_bias_) {
            quantized.push_back(bias);
        }

        for (const std::uint32_t feature : active_feature_indices(board, perspective, info_.feature_set)) {
            const bool placement = feature < info_.placement_feature_count;
            const std::uint32_t weight_feature = placement ? feature : feature - info_.placement_feature_count;
            const std::size_t offset = static_cast<std::size_t>(weight_feature) * info_.hidden_size;
            if (placement) {
                for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
                    quantized[index] += placement_feature_weights_quantized_[offset + index];
                }
            } else {
                for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
                    quantized[index] += threat_feature_weights_quantized_[offset + index];
                }
            }
        }

        std::vector<float> values;
        values.reserve(info_.hidden_size);
        const float scale = static_cast<float>(info_.accumulator_scale);
        for (const int value : quantized) {
            values.push_back(static_cast<float>(value) / scale);
        }
        return values;
    }

    std::vector<float> values = hidden_bias_float_;
    for (const std::uint32_t feature : active_feature_indices(board, perspective, info_.feature_set)) {
        const std::vector<float>* weights = &feature_weights_float_;
        std::uint32_t weight_feature = feature;
        if (info_.feature_set == FeatureSet::HalfKaV2HmFullThreats) {
            if (feature < info_.placement_feature_count) {
                weights = &placement_feature_weights_float_;
            } else {
                weights = &threat_feature_weights_float_;
                weight_feature = feature - info_.placement_feature_count;
            }
        }
        const std::size_t offset = static_cast<std::size_t>(weight_feature) * info_.hidden_size;
        for (std::uint32_t index = 0; index < info_.hidden_size; ++index) {
            values[index] += (*weights)[offset + index];
        }
    }
    return values;
}

std::vector<float> Network::accumulator_float_from_quantized(
    const Board& board,
    Color perspective,
    const std::vector<int>& quantized_accumulator
) const {
    (void) board;
    (void) perspective;
    std::vector<float> values;
    values.reserve(info_.hidden_size);
    const float scale = static_cast<float>(info_.accumulator_scale);
    for (const int value : quantized_accumulator) {
        values.push_back(static_cast<float>(value) / scale);
    }
    return values;
}

int Network::evaluate_sf_lite_white_perspective(
    const Board& board,
    const std::vector<float>& white_input,
    const std::vector<float>& black_input,
    ProfileCounters* profile
) const {
    thread_local std::vector<float> white;
    thread_local std::vector<float> black;
    thread_local std::vector<float> fc0;
    thread_local std::vector<float> fc1;
    const auto convert_start = profile != nullptr ? ProfileClock::now() : ProfileClock::time_point{};
    white = white_input;
    black = black_input;
    for (float& value : white) {
        value = std::clamp(value, 0.0F, 1.0F);
    }
    for (float& value : black) {
        value = std::clamp(value, 0.0F, 1.0F);
    }
    if (profile != nullptr) {
        profile->dense_convert_ns += elapsed_ns_since(convert_start);
    }
    return evaluate_sf_lite_white_perspective_clamped(board, white, black, fc0, fc1, profile);
}

int Network::evaluate_sf_lite_white_perspective_quantized(
    const Board& board,
    const std::vector<int>& white_input,
    const std::vector<int>& black_input,
    ProfileCounters* profile
) const {
    thread_local std::vector<float> white;
    thread_local std::vector<float> black;
    thread_local std::vector<float> fc0;
    thread_local std::vector<float> fc1;
    const auto convert_start = profile != nullptr ? ProfileClock::now() : ProfileClock::time_point{};
    clamp_quantized_to_float(white_input, white, info_.hidden_size, info_.accumulator_scale);
    clamp_quantized_to_float(black_input, black, info_.hidden_size, info_.accumulator_scale);
    if (profile != nullptr) {
        profile->dense_convert_ns += elapsed_ns_since(convert_start);
    }
    return evaluate_sf_lite_white_perspective_clamped(board, white, black, fc0, fc1, profile);
}

int Network::evaluate_sf_lite_white_perspective_clamped(
    const Board& board,
    const std::vector<float>& white,
    const std::vector<float>& black,
    std::vector<float>& fc0,
    std::vector<float>& fc1,
    ProfileCounters* profile
) const {
    const auto dense_start = profile != nullptr ? ProfileClock::now() : ProfileClock::time_point{};
    const bool white_to_move = board.side_to_move() == Color::White;
    const std::vector<float>& stm = white_to_move ? white : black;
    const std::vector<float>& nstm = white_to_move ? black : white;

    float direct = direct_bias_
        + dot_product_float(stm.data(), direct_weights_.data(), info_.hidden_size)
        + dot_product_float(nstm.data(), direct_weights_.data() + info_.hidden_size, info_.hidden_size);

    fc0.assign(info_.l2_size, 0.0F);
    for (std::uint32_t row = 0; row < info_.l2_size; ++row) {
        const std::size_t row_offset = static_cast<std::size_t>(row) * info_.hidden_size * 2;
        const float value = fc0_bias_[row]
            + dot_product_float(stm.data(), fc0_weights_.data() + row_offset, info_.hidden_size)
            + dot_product_float(nstm.data(), fc0_weights_.data() + row_offset + info_.hidden_size, info_.hidden_size);
        fc0[row] = std::clamp(value, 0.0F, 1.0F);
    }

    fc1.assign(info_.l3_size, 0.0F);
    for (std::uint32_t row = 0; row < info_.l3_size; ++row) {
        const std::size_t row_offset = static_cast<std::size_t>(row) * info_.l2_size * 2;
        float value = fc1_bias_[row]
            + dot_product_square_float(fc0.data(), fc1_weights_.data() + row_offset, info_.l2_size);
        value += dot_product_float(fc0.data(), fc1_weights_.data() + row_offset + info_.l2_size, info_.l2_size);
        fc1[row] = std::clamp(value, 0.0F, 1.0F);
    }

    const float side_to_move_score = direct + fc2_bias_ + side_to_move_weight_float_
        + dot_product_float(fc1.data(), fc2_weights_.data(), info_.l3_size);
    const int rounded = static_cast<int>(std::lround(side_to_move_score));
    if (profile != nullptr) {
        profile->dense_forward_ns += elapsed_ns_since(dense_start);
    }
    return white_to_move ? rounded : -rounded;
}

int Network::evaluate_sf_lite_white_perspective(const Board& board) const {
    return evaluate_sf_lite_white_perspective(
        board,
        accumulator_float(board, Color::White),
        accumulator_float(board, Color::Black)
    );
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

int Network::evaluate_white_perspective(
    const Board& board,
    const QuantizedAccumulatorPair& accumulator_pair,
    ProfileCounters* profile
) const {
    if (!supports_quantized_accumulator_stack() || !accumulator_pair.valid) {
        return evaluate_white_perspective(board);
    }
    return evaluate_sf_lite_white_perspective_quantized(
        board,
        accumulator_pair.white,
        accumulator_pair.black,
        profile
    );
}

}  // namespace chess::engine::nnue
