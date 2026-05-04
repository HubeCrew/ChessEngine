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
        FEATURE_SET_HALFKA_V2_HM_THREAT_LITE,
        FORMAT_VERSION_FEATURE_SET,
        HALFKA_V2_HM_LITE_FEATURE_COUNT,
        HALFKA_V2_HM_THREAT_LITE_FEATURE_COUNT,
        NnueModel,
        active_features,
        evaluate_checkpoint_white_perspective,
        export_binary,
        feature_count_for,
        feature_index,
        load_binary,
        load_checkpoint,
        save_checkpoint,
        threat_feature_index,
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

    if feature_count_for(FEATURE_SET_HALFKA_V2_HM_THREAT_LITE) != 71680:
        print("bad HalfKAv2_hm-threat-lite feature count", file=sys.stderr)
        return 1

    threatened = chess.Board("4k3/8/8/8/7q/5N2/8/4K3 w - - 0 1")
    quiet = chess.Board("4k3/8/8/8/7q/6N1/8/4K3 w - - 0 1")
    threat_index = threat_feature_index(
        chess.WHITE,
        chess.E1,
        chess.KNIGHT,
        chess.QUEEN,
        chess.H4,
    )
    threatened_features = active_features(threatened, chess.WHITE, FEATURE_SET_HALFKA_V2_HM_THREAT_LITE)
    quiet_features = active_features(quiet, chess.WHITE, FEATURE_SET_HALFKA_V2_HM_THREAT_LITE)
    if threat_index not in threatened_features:
        print("missing knight-on-queen threat feature", file=sys.stderr)
        return 1
    if threat_index in quiet_features:
        print("stale threat feature after moving attacker away", file=sys.stderr)
        return 1
    if threat_index is None or threat_index < HALFKA_V2_HM_LITE_FEATURE_COUNT or threat_index >= HALFKA_V2_HM_THREAT_LITE_FEATURE_COUNT:
        print(f"threat index out of range: {threat_index}", file=sys.stderr)
        return 1

    mirrored = chess.Board("3k4/8/8/8/q7/2N5/8/3K4 w - - 0 1")
    if threatened_features != active_features(mirrored, chess.WHITE, FEATURE_SET_HALFKA_V2_HM_THREAT_LITE):
        print("horizontal threat mirroring failed", file=sys.stderr)
        return 1

    black_perspective = chess.Board("4k3/8/5n2/7Q/8/8/8/4K3 b - - 0 1")
    if threatened_features != active_features(black_perspective, chess.BLACK, FEATURE_SET_HALFKA_V2_HM_THREAT_LITE):
        print("black perspective threat orientation failed", file=sys.stderr)
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

        threat_checkpoint = root / "threat_model.pt"
        threat_binary_path = root / "threat_model.nnue"
        threat_model = NnueModel(hidden_size=8, feature_set=FEATURE_SET_HALFKA_V2_HM_THREAT_LITE)
        save_checkpoint(threat_checkpoint, threat_model, {"test": "threat"})
        loaded_threat, threat_metadata = load_checkpoint(threat_checkpoint, torch.device("cpu"))
        if loaded_threat.feature_set != FEATURE_SET_HALFKA_V2_HM_THREAT_LITE or threat_metadata.get("test") != "threat":
            print("threat checkpoint feature-set metadata was not preserved", file=sys.stderr)
            return 1
        export_binary(threat_binary_path, loaded_threat)
        threat_binary = load_binary(threat_binary_path)
        if threat_binary.feature_set != FEATURE_SET_HALFKA_V2_HM_THREAT_LITE:
            print("threat binary feature set was not preserved", file=sys.stderr)
            return 1
        if threat_binary.feature_count != HALFKA_V2_HM_THREAT_LITE_FEATURE_COUNT:
            print("threat binary feature count was not preserved", file=sys.stderr)
            return 1
        checkpoint_score = evaluate_checkpoint_white_perspective(loaded_threat, threatened, torch.device("cpu"))
        binary_score = threat_binary.evaluate_white_perspective(threatened)
        if int(round(checkpoint_score)) != binary_score:
            print(f"checkpoint/binary parity failed: {checkpoint_score} != {binary_score}", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
