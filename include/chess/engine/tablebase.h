#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "chess/core/board.h"

namespace chess::engine::tablebase {

enum class Wdl {
    Loss = 0,
    BlessedLoss = 1,
    Draw = 2,
    CursedWin = 3,
    Win = 4,
};

struct ProbeResult {
    Wdl wdl = Wdl::Draw;
    int score = 0;
};

struct RootMove {
    Move move;
    Wdl wdl = Wdl::Draw;
    int dtz = 0;
    int rank = 0;
    int score = 0;
};

struct RootProbeResult {
    Move best_move;
    Wdl wdl = Wdl::Draw;
    int dtz = 0;
    int score = 0;
    std::vector<RootMove> moves;
};

[[nodiscard]] bool available();
[[nodiscard]] bool initialized();
[[nodiscard]] unsigned largest();
[[nodiscard]] std::filesystem::path path();
[[nodiscard]] bool initialize(const std::filesystem::path& tablebase_path, std::string* error = nullptr);
void clear();

[[nodiscard]] bool probeable(const Board& board, bool root_probe);
[[nodiscard]] std::optional<ProbeResult> probe_wdl(const Board& board);
[[nodiscard]] std::optional<RootProbeResult> probe_root(const Board& board);
[[nodiscard]] const char* wdl_name(Wdl wdl);

}  // namespace chess::engine::tablebase
