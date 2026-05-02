#include "chess/core/zobrist.h"

#include <array>

namespace chess::zobrist {

namespace {

constexpr std::uint64_t splitmix64(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

constexpr int piece_index(Piece piece) {
    return static_cast<int>(piece) - static_cast<int>(Piece::WhitePawn);
}

constexpr std::array<std::uint64_t, 12 * 64> make_piece_square_keys() {
    std::array<std::uint64_t, 12 * 64> keys{};
    for (std::size_t index = 0; index < keys.size(); ++index) {
        keys[index] = splitmix64(0x3243f6a8885a308dULL + index);
    }
    return keys;
}

constexpr std::array<std::uint64_t, 16> make_castling_keys() {
    std::array<std::uint64_t, 16> keys{};
    for (std::size_t index = 0; index < keys.size(); ++index) {
        keys[index] = splitmix64(0x13198a2e03707344ULL + index);
    }
    return keys;
}

constexpr std::array<std::uint64_t, 8> make_en_passant_keys() {
    std::array<std::uint64_t, 8> keys{};
    for (std::size_t index = 0; index < keys.size(); ++index) {
        keys[index] = splitmix64(0xa4093822299f31d0ULL + index);
    }
    return keys;
}

constexpr auto kPieceSquareKeys = make_piece_square_keys();
constexpr auto kCastlingKeys = make_castling_keys();
constexpr auto kEnPassantKeys = make_en_passant_keys();
constexpr std::uint64_t kSideToMoveKey = splitmix64(0x082efa98ec4e6c89ULL);

}  // namespace

std::uint64_t piece_square(Piece piece, Square square) {
    if (piece == Piece::None || !is_valid_square(square)) {
        return 0;
    }
    return kPieceSquareKeys[static_cast<std::size_t>(piece_index(piece) * 64 + square)];
}

std::uint64_t side_to_move() {
    return kSideToMoveKey;
}

std::uint64_t castling(std::uint8_t rights) {
    return kCastlingKeys[rights & 0x0fU];
}

std::uint64_t en_passant_file(int file) {
    if (file < 0 || file > 7) {
        return 0;
    }
    return kEnPassantKeys[static_cast<std::size_t>(file)];
}

}  // namespace chess::zobrist

