#include "chess/engine/tablebase.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <mutex>

#ifdef CHESSENGINE_HAS_FATHOM
#include <tbprobe.h>
#endif

namespace chess::engine::tablebase {

namespace {

std::mutex g_tablebase_mutex;
bool g_initialized = false;
std::filesystem::path g_path;

int piece_count(const Board& board) {
    return __builtin_popcountll(board.occupancy());
}

unsigned en_passant_square_for_fathom(const Board& board) {
    return board.en_passant_square() == kNoSquare ? 0U : static_cast<unsigned>(board.en_passant_square());
}

PieceType promotion_from_fathom(unsigned promotion) {
    switch (promotion) {
        case 1:
            return PieceType::Queen;
        case 2:
            return PieceType::Rook;
        case 3:
            return PieceType::Bishop;
        case 4:
            return PieceType::Knight;
        default:
            return PieceType::None;
    }
}

std::uint8_t flags_for_tablebase_move(const Board& board, Square from, Square to, PieceType promotion) {
    std::uint8_t flags = Quiet;
    const Piece moved = board.piece_at(from);
    const Piece captured = board.piece_at(to);
    if (captured != Piece::None) {
        flags |= Capture;
    }
    if (type_of(moved) == PieceType::Pawn && to == board.en_passant_square() && file_of(from) != file_of(to) && captured == Piece::None) {
        flags |= Capture | EnPassant;
    }
    if (promotion != PieceType::None) {
        flags |= Promotion;
    }
    if (type_of(moved) == PieceType::Pawn && std::abs(rank_of(to) - rank_of(from)) == 2) {
        flags |= DoublePawnPush;
    }
    return flags;
}

Move move_from_fathom_move(const Board& board, TbMove move) {
    const Square from = static_cast<Square>(TB_MOVE_FROM(move));
    const Square to = static_cast<Square>(TB_MOVE_TO(move));
    const PieceType promotion = promotion_from_fathom(TB_MOVE_PROMOTES(move));
    return Move{from, to, promotion, flags_for_tablebase_move(board, from, to, promotion)};
}

Wdl wdl_from_fathom(unsigned wdl) {
    switch (wdl) {
        case TB_LOSS:
            return Wdl::Loss;
        case TB_BLESSED_LOSS:
            return Wdl::BlessedLoss;
        case TB_DRAW:
            return Wdl::Draw;
        case TB_CURSED_WIN:
            return Wdl::CursedWin;
        case TB_WIN:
            return Wdl::Win;
        default:
            return Wdl::Draw;
    }
}

int score_from_wdl(Wdl wdl) {
    switch (wdl) {
        case Wdl::Loss:
            return -20000;
        case Wdl::BlessedLoss:
            return -10000;
        case Wdl::Draw:
            return 0;
        case Wdl::CursedWin:
            return 10000;
        case Wdl::Win:
            return 20000;
    }
    return 0;
}

struct FathomPosition {
    std::uint64_t white = 0;
    std::uint64_t black = 0;
    std::uint64_t kings = 0;
    std::uint64_t queens = 0;
    std::uint64_t rooks = 0;
    std::uint64_t bishops = 0;
    std::uint64_t knights = 0;
    std::uint64_t pawns = 0;
    unsigned rule50 = 0;
    unsigned castling = 0;
    unsigned ep = 0;
    bool turn = true;
};

FathomPosition to_fathom_position(const Board& board) {
    return FathomPosition{
        board.occupancy(Color::White),
        board.occupancy(Color::Black),
        board.pieces(Color::White, PieceType::King) | board.pieces(Color::Black, PieceType::King),
        board.pieces(Color::White, PieceType::Queen) | board.pieces(Color::Black, PieceType::Queen),
        board.pieces(Color::White, PieceType::Rook) | board.pieces(Color::Black, PieceType::Rook),
        board.pieces(Color::White, PieceType::Bishop) | board.pieces(Color::Black, PieceType::Bishop),
        board.pieces(Color::White, PieceType::Knight) | board.pieces(Color::Black, PieceType::Knight),
        board.pieces(Color::White, PieceType::Pawn) | board.pieces(Color::Black, PieceType::Pawn),
        static_cast<unsigned>(std::max(0, board.halfmove_clock())),
        board.castling_rights(),
        en_passant_square_for_fathom(board),
        board.side_to_move() == Color::White,
    };
}

}  // namespace

bool available() {
#ifdef CHESSENGINE_HAS_FATHOM
    return true;
#else
    return false;
#endif
}

bool initialized() {
    std::lock_guard lock(g_tablebase_mutex);
    return g_initialized;
}

unsigned largest() {
#ifdef CHESSENGINE_HAS_FATHOM
    std::lock_guard lock(g_tablebase_mutex);
    return TB_LARGEST;
#else
    return 0;
#endif
}

std::filesystem::path path() {
    std::lock_guard lock(g_tablebase_mutex);
    return g_path;
}

bool initialize(const std::filesystem::path& tablebase_path, std::string* error) {
#ifdef CHESSENGINE_HAS_FATHOM
    std::lock_guard lock(g_tablebase_mutex);
    tb_free();
    g_initialized = false;
    g_path.clear();
    if (tablebase_path.empty()) {
        return true;
    }
    if (!tb_init(tablebase_path.string().c_str())) {
        if (error != nullptr) {
            *error = "tb_init failed for path " + tablebase_path.string();
        }
        return false;
    }
    g_initialized = TB_LARGEST > 0;
    g_path = tablebase_path;
    if (!g_initialized && error != nullptr) {
        *error = "no Syzygy tablebase files found at " + tablebase_path.string();
    }
    return g_initialized;
#else
    if (error != nullptr) {
        *error = "engine was built without Fathom support";
    }
    return false;
#endif
}

void clear() {
#ifdef CHESSENGINE_HAS_FATHOM
    std::lock_guard lock(g_tablebase_mutex);
    tb_free();
#endif
    g_initialized = false;
    g_path.clear();
}

bool probeable(const Board& board, bool root_probe) {
#ifdef CHESSENGINE_HAS_FATHOM
    if (!g_initialized || TB_LARGEST == 0 || piece_count(board) > static_cast<int>(TB_LARGEST)) {
        return false;
    }
    if (board.castling_rights() != 0) {
        return false;
    }
    if (!root_probe && board.halfmove_clock() != 0) {
        return false;
    }
    return true;
#else
    (void)board;
    (void)root_probe;
    return false;
#endif
}

std::optional<ProbeResult> probe_wdl(const Board& board) {
#ifdef CHESSENGINE_HAS_FATHOM
    if (!probeable(board, false)) {
        return std::nullopt;
    }
    const FathomPosition position = to_fathom_position(board);
    const unsigned result = tb_probe_wdl(
        position.white,
        position.black,
        position.kings,
        position.queens,
        position.rooks,
        position.bishops,
        position.knights,
        position.pawns,
        position.rule50,
        position.castling,
        position.ep,
        position.turn
    );
    if (result == TB_RESULT_FAILED) {
        return std::nullopt;
    }
    const Wdl wdl = wdl_from_fathom(result);
    return ProbeResult{wdl, score_from_wdl(wdl)};
#else
    (void)board;
    return std::nullopt;
#endif
}

std::optional<RootProbeResult> probe_root(const Board& board) {
#ifdef CHESSENGINE_HAS_FATHOM
    if (!probeable(board, true)) {
        return std::nullopt;
    }
    const FathomPosition position = to_fathom_position(board);
    TbRootMoves root_moves{};
    int ok = tb_probe_root_dtz(
        position.white,
        position.black,
        position.kings,
        position.queens,
        position.rooks,
        position.bishops,
        position.knights,
        position.pawns,
        position.rule50,
        position.castling,
        position.ep,
        position.turn,
        false,
        true,
        &root_moves
    );
    if (ok == 0) {
        root_moves = TbRootMoves{};
        ok = tb_probe_root_wdl(
            position.white,
            position.black,
            position.kings,
            position.queens,
            position.rooks,
            position.bishops,
            position.knights,
            position.pawns,
            position.rule50,
            position.castling,
            position.ep,
            position.turn,
            true,
            &root_moves
        );
    }
    if (ok == 0 || root_moves.size == 0) {
        return std::nullopt;
    }

    RootProbeResult result;
    result.moves.reserve(root_moves.size);
    for (unsigned index = 0; index < root_moves.size; ++index) {
        const TbRootMove& tb_move = root_moves.moves[index];
        RootMove root_move;
        root_move.move = move_from_fathom_move(board, tb_move.move);
        root_move.rank = tb_move.tbRank;
        root_move.score = tb_move.tbScore;
        if (tb_move.tbScore > 0) {
            root_move.wdl = Wdl::Win;
        } else if (tb_move.tbScore < 0) {
            root_move.wdl = Wdl::Loss;
        } else {
            root_move.wdl = Wdl::Draw;
        }
        result.moves.push_back(root_move);
    }
    std::sort(result.moves.begin(), result.moves.end(), [](const RootMove& lhs, const RootMove& rhs) {
        if (lhs.rank != rhs.rank) {
            return lhs.rank > rhs.rank;
        }
        return lhs.score > rhs.score;
    });
    result.best_move = result.moves.front().move;
    result.wdl = result.moves.front().wdl;
    result.score = result.moves.front().score;
    return result;
#else
    (void)board;
    return std::nullopt;
#endif
}

const char* wdl_name(Wdl wdl) {
    switch (wdl) {
        case Wdl::Loss:
            return "loss";
        case Wdl::BlessedLoss:
            return "blessed-loss";
        case Wdl::Draw:
            return "draw";
        case Wdl::CursedWin:
            return "cursed-win";
        case Wdl::Win:
            return "win";
    }
    return "draw";
}

}  // namespace chess::engine::tablebase
