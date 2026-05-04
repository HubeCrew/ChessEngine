#!/usr/bin/env python3
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

try:
    import chess
    import torch
except ImportError:
    print("python-chess or torch not installed; skipping")
    raise SystemExit(0)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: nnue_feature_set_smoke_test.py <tools_dir>", file=sys.stderr)
        return 2
    sys.path.insert(0, str(Path(sys.argv[1])))
    from nnue_model import (  # noqa: PLC0415
        FEATURE_SET_HALFKA_V2_HM_LITE,
        FORMAT_VERSION_FEATURE_SET,
        HALFKA_V2_HM_LITE_FEATURE_COUNT,
        NnueModel,
        active_features,
        export_binary,
        feature_index,
        load_binary,
        load_checkpoint,
        save_checkpoint,
    )

    kingside = feature_index(
        chess.WHITE,
        chess.E1,
        chess.Piece(chess.KNIGHT, chess.WHITE),
        chess.G3,
        FEATURE_SET_HALFKA_V2_HM_LITE,
    )
    queenside = feature_index(
        chess.WHITE,
        chess.D1,
        chess.Piece(chess.KNIGHT, chess.WHITE),
        chess.B3,
        FEATURE_SET_HALFKA_V2_HM_LITE,
    )
    if kingside != queenside:
        print(f"horizontal king mirroring failed: {kingside} != {queenside}", file=sys.stderr)
        return 1
    if kingside is None or kingside >= HALFKA_V2_HM_LITE_FEATURE_COUNT:
        print(f"feature index out of range: {kingside}", file=sys.stderr)
        return 1

    features = active_features(chess.Board(), chess.WHITE, FEATURE_SET_HALFKA_V2_HM_LITE)
    if len(features) != 30 or max(features) >= HALFKA_V2_HM_LITE_FEATURE_COUNT:
        print("bad start-position HalfKAv2_hm-lite features", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        checkpoint = root / "model.pt"
        binary_path = root / "model.nnue"
        model = NnueModel(hidden_size=8, feature_set=FEATURE_SET_HALFKA_V2_HM_LITE)
        save_checkpoint(checkpoint, model, {"test": "halfka"})
        loaded, metadata = load_checkpoint(checkpoint, torch.device("cpu"))
        if loaded.feature_set != FEATURE_SET_HALFKA_V2_HM_LITE or metadata.get("test") != "halfka":
            print("checkpoint feature-set metadata was not preserved", file=sys.stderr)
            return 1
        if loaded.format_version != FORMAT_VERSION_FEATURE_SET:
            print(f"expected v5 feature-set checkpoint, got v{loaded.format_version}", file=sys.stderr)
            return 1
        export_binary(binary_path, loaded)
        binary = load_binary(binary_path)
        if binary.feature_set != FEATURE_SET_HALFKA_V2_HM_LITE:
            print("binary feature set was not preserved", file=sys.stderr)
            return 1
        if binary.feature_count != HALFKA_V2_HM_LITE_FEATURE_COUNT:
            print("binary feature count was not preserved", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
