from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

import chess
import torch
from torch import nn


FORMAT_VERSION_FIXED_PERSPECTIVE = 2
FORMAT_VERSION_SIDE_TO_MOVE_PERSPECTIVE = 3
FORMAT_VERSION_SF_LITE = 4
FORMAT_VERSION_FEATURE_SET = 5
FORMAT_VERSION = FORMAT_VERSION_SF_LITE
SUPPORTED_FORMAT_VERSIONS = (1, 2, 3, 4, 5)
FEATURE_SET_HALFKP = "halfkp-v1"
FEATURE_SET_HALFKA_V2_HM_LITE = "halfka-v2-hm-lite"
FEATURE_SET_HALFKA_V2_HM_THREAT_LITE = "halfka-v2-hm-threat-lite"
SUPPORTED_FEATURE_SETS = (FEATURE_SET_HALFKP, FEATURE_SET_HALFKA_V2_HM_LITE, FEATURE_SET_HALFKA_V2_HM_THREAT_LITE)
FEATURE_SET_IDS = {
    FEATURE_SET_HALFKP: 1,
    FEATURE_SET_HALFKA_V2_HM_LITE: 2,
    FEATURE_SET_HALFKA_V2_HM_THREAT_LITE: 3,
}
FEATURE_SET_BY_ID = {value: key for key, value in FEATURE_SET_IDS.items()}
HALFKP_FEATURE_COUNT = 64 * 10 * 64
HALFKA_V2_HM_LITE_KING_BUCKETS = 32
HALFKA_V2_HM_LITE_FEATURE_COUNT = HALFKA_V2_HM_LITE_KING_BUCKETS * 10 * 64
HALFKA_V2_HM_THREAT_LITE_THREAT_FEATURE_COUNT = HALFKA_V2_HM_LITE_KING_BUCKETS * 5 * 5 * 64
HALFKA_V2_HM_THREAT_LITE_FEATURE_COUNT = (
    HALFKA_V2_HM_LITE_FEATURE_COUNT + HALFKA_V2_HM_THREAT_LITE_THREAT_FEATURE_COUNT
)
FEATURE_COUNT = HALFKP_FEATURE_COUNT
DEFAULT_HIDDEN_SIZE = 256
DEFAULT_L2_SIZE = 64
DEFAULT_L3_SIZE = 32
DEFAULT_ACCUMULATOR_SCALE = 256
DEFAULT_OUTPUT_SCALE = 1024
MAGIC = b"CENNUE1\0"
ARCHITECTURE_LINEAR = "linear"
ARCHITECTURE_SF_LITE = "sf-lite"

PIECE_SLOT = {
    chess.PAWN: 0,
    chess.KNIGHT: 1,
    chess.BISHOP: 2,
    chess.ROOK: 3,
    chess.QUEEN: 4,
}


def rounded_divide(value: int, scale: int) -> int:
    if value >= 0:
        return (value + scale // 2) // scale
    return -((-value + scale // 2) // scale)


def perspective_square(square: chess.Square, perspective: chess.Color) -> chess.Square:
    if perspective == chess.WHITE:
        return square
    return chess.square(chess.square_file(square), 7 - chess.square_rank(square))


def horizontal_mirror_square(square: chess.Square) -> chess.Square:
    return chess.square(7 - chess.square_file(square), chess.square_rank(square))


def halfka_v2_hm_orientation(perspective_king: chess.Square, perspective: chess.Color) -> tuple[int, bool]:
    king_square = perspective_square(perspective_king, perspective)
    mirror = chess.square_file(king_square) >= 4
    if mirror:
        king_square = horizontal_mirror_square(king_square)
    king_bucket = chess.square_rank(king_square) * 4 + chess.square_file(king_square)
    return king_bucket, mirror


def halfka_v2_hm_oriented_square(square: chess.Square, perspective: chess.Color, mirror: bool) -> chess.Square:
    oriented = perspective_square(square, perspective)
    return horizontal_mirror_square(oriented) if mirror else oriented


def feature_count_for(feature_set: str) -> int:
    if feature_set == FEATURE_SET_HALFKP:
        return HALFKP_FEATURE_COUNT
    if feature_set == FEATURE_SET_HALFKA_V2_HM_LITE:
        return HALFKA_V2_HM_LITE_FEATURE_COUNT
    if feature_set == FEATURE_SET_HALFKA_V2_HM_THREAT_LITE:
        return HALFKA_V2_HM_THREAT_LITE_FEATURE_COUNT
    raise ValueError(f"unsupported NNUE feature set: {feature_set}")


def feature_set_for_format_version(format_version: int, feature_set: str | None = None) -> str:
    if feature_set is not None:
        if feature_set not in SUPPORTED_FEATURE_SETS:
            raise ValueError(f"unsupported NNUE feature set: {feature_set}")
        return feature_set
    if format_version >= FORMAT_VERSION_FEATURE_SET:
        raise ValueError("feature-set-aware NNUE format requires explicit feature_set metadata")
    return FEATURE_SET_HALFKP


def feature_set_id(feature_set: str) -> int:
    try:
        return FEATURE_SET_IDS[feature_set]
    except KeyError as exc:
        raise ValueError(f"unsupported NNUE feature set: {feature_set}") from exc


def feature_set_from_id(identifier: int) -> str:
    try:
        return FEATURE_SET_BY_ID[identifier]
    except KeyError as exc:
        raise ValueError(f"unsupported NNUE feature set id {identifier}") from exc


def normalize_feature_set(feature_set: str | None) -> str:
    if feature_set is None:
        return FEATURE_SET_HALFKP
    if feature_set not in SUPPORTED_FEATURE_SETS:
        raise ValueError(f"unsupported NNUE feature set: {feature_set}")
    return feature_set


def feature_index(
    perspective: chess.Color,
    perspective_king: chess.Square,
    piece: chess.Piece,
    square: chess.Square,
    feature_set: str = FEATURE_SET_HALFKP,
) -> int | None:
    if piece.piece_type == chess.KING:
        return None
    slot = PIECE_SLOT.get(piece.piece_type)
    if slot is None:
        return None
    relative_color = 0 if piece.color == perspective else 1
    piece_slot = relative_color * 5 + slot
    king_square = perspective_square(perspective_king, perspective)
    piece_square = perspective_square(square, perspective)
    if feature_set == FEATURE_SET_HALFKP:
        return ((king_square * 10 + piece_slot) * 64) + piece_square
    if feature_set in (FEATURE_SET_HALFKA_V2_HM_LITE, FEATURE_SET_HALFKA_V2_HM_THREAT_LITE):
        king_bucket, mirror = halfka_v2_hm_orientation(perspective_king, perspective)
        piece_square = halfka_v2_hm_oriented_square(square, perspective, mirror)
        return ((king_bucket * 10 + piece_slot) * 64) + piece_square
    raise ValueError(f"unsupported NNUE feature set: {feature_set}")


def threat_feature_index(
    perspective: chess.Color,
    perspective_king: chess.Square,
    attacker_type: chess.PieceType,
    victim_type: chess.PieceType,
    target_square: chess.Square,
) -> int | None:
    attacker_slot = PIECE_SLOT.get(attacker_type)
    victim_slot = PIECE_SLOT.get(victim_type)
    if attacker_slot is None or victim_slot is None:
        return None
    king_bucket, mirror = halfka_v2_hm_orientation(perspective_king, perspective)
    oriented_target = halfka_v2_hm_oriented_square(target_square, perspective, mirror)
    return HALFKA_V2_HM_LITE_FEATURE_COUNT + (((king_bucket * 5 + attacker_slot) * 5 + victim_slot) * 64 + oriented_target)


def active_threat_features(board: chess.Board, perspective: chess.Color, perspective_king: chess.Square) -> list[int]:
    features: set[int] = set()
    for target_square, victim in board.piece_map().items():
        if victim.color == perspective or victim.piece_type == chess.KING:
            continue
        attacker_types: set[chess.PieceType] = set()
        for attacker_square in board.attackers(perspective, target_square):
            attacker = board.piece_at(attacker_square)
            if attacker is None or attacker.piece_type == chess.KING:
                continue
            attacker_types.add(attacker.piece_type)
        for attacker_type in attacker_types:
            index = threat_feature_index(perspective, perspective_king, attacker_type, victim.piece_type, target_square)
            if index is not None:
                features.add(index)
    return sorted(features)


def active_features(board: chess.Board, perspective: chess.Color, feature_set: str = FEATURE_SET_HALFKP) -> list[int]:
    king = board.king(perspective)
    if king is None:
        return []
    features: list[int] = []
    for square, piece in board.piece_map().items():
        index = feature_index(perspective, king, piece, square, feature_set)
        if index is not None:
            features.append(index)
    if feature_set == FEATURE_SET_HALFKA_V2_HM_THREAT_LITE:
        features.extend(active_threat_features(board, perspective, king))
    features.sort()
    return features


@dataclass(frozen=True)
class BinaryNnue:
    format_version: int
    feature_count: int
    feature_set: str
    hidden_size: int
    accumulator_scale: int
    output_scale: int
    hidden_bias: torch.Tensor
    feature_weights: torch.Tensor
    output_weights: torch.Tensor
    output_bias: int
    side_to_move_weight: int = 0
    architecture: str = ARCHITECTURE_LINEAR
    l2_size: int = 0
    l3_size: int = 0
    hidden_bias_float: torch.Tensor | None = None
    feature_weights_float: torch.Tensor | None = None
    direct_weights: torch.Tensor | None = None
    direct_bias: float = 0.0
    fc0_weight: torch.Tensor | None = None
    fc0_bias: torch.Tensor | None = None
    fc1_weight: torch.Tensor | None = None
    fc1_bias: torch.Tensor | None = None
    fc2_weight: torch.Tensor | None = None
    fc2_bias: float = 0.0
    side_to_move_weight_float: float = 0.0

    def accumulator(self, features: list[int]) -> torch.Tensor:
        if self.architecture == ARCHITECTURE_SF_LITE:
            assert self.hidden_bias_float is not None
            assert self.feature_weights_float is not None
            if features:
                indices = torch.tensor(features, dtype=torch.long)
                return self.hidden_bias_float + self.feature_weights_float[indices].sum(dim=0)
            return self.hidden_bias_float.clone()
        if features:
            indices = torch.tensor(features, dtype=torch.long)
            return self.hidden_bias.to(torch.int32) + self.feature_weights[indices].to(torch.int32).sum(dim=0)
        return self.hidden_bias.to(torch.int32)

    def evaluate_white_perspective(self, board: chess.Board) -> int:
        side_to_move = 1 if board.turn == chess.WHITE else -1
        if self.architecture == ARCHITECTURE_SF_LITE:
            assert self.direct_weights is not None
            assert self.fc0_weight is not None
            assert self.fc0_bias is not None
            assert self.fc1_weight is not None
            assert self.fc1_bias is not None
            assert self.fc2_weight is not None
            white = self.accumulator(active_features(board, chess.WHITE, self.feature_set)).clamp(0.0, 1.0)
            black = self.accumulator(active_features(board, chess.BLACK, self.feature_set)).clamp(0.0, 1.0)
            stm = white if board.turn == chess.WHITE else black
            nstm = black if board.turn == chess.WHITE else white
            inputs = torch.cat([stm, nstm])
            direct = self.direct_bias + float((inputs * self.direct_weights).sum())
            fc0 = torch.clamp(torch.mv(self.fc0_weight, inputs) + self.fc0_bias, 0.0, 1.0)
            fc1_input = torch.cat([fc0 * fc0, fc0])
            fc1 = torch.clamp(torch.mv(self.fc1_weight, fc1_input) + self.fc1_bias, 0.0, 1.0)
            stm_score = direct + float((fc1 * self.fc2_weight).sum()) + self.fc2_bias + self.side_to_move_weight_float
            white_score = stm_score if side_to_move > 0 else -stm_score
            return int(round(white_score))

        white = self.accumulator(active_features(board, chess.WHITE, self.feature_set)).clamp(0, self.accumulator_scale)
        black = self.accumulator(active_features(board, chess.BLACK, self.feature_set)).clamp(0, self.accumulator_scale)
        if self.format_version >= FORMAT_VERSION_SIDE_TO_MOVE_PERSPECTIVE:
            stm = white if board.turn == chess.WHITE else black
            nstm = black if board.turn == chess.WHITE else white
            activations = torch.cat([stm, nstm]).to(torch.int64)
            total = int(self.output_bias + int((activations * self.output_weights.to(torch.int64)).sum()) + self.side_to_move_weight)
            score = rounded_divide(total, self.output_scale)
            return score if side_to_move > 0 else -score

        activations = torch.cat([white, black]).to(torch.int64)
        total = int(
            self.output_bias
            + int((activations * self.output_weights.to(torch.int64)).sum())
            + side_to_move * self.side_to_move_weight
        )
        return rounded_divide(total, self.output_scale)


@dataclass(frozen=True)
class Batch:
    white: torch.Tensor
    white_mask: torch.Tensor
    black: torch.Tensor
    black_mask: torch.Tensor
    side_to_move: torch.Tensor
    target: torch.Tensor


class NnueModel(nn.Module):
    def __init__(
        self,
        hidden_size: int = DEFAULT_HIDDEN_SIZE,
        perspective_mode: str = "side-to-move",
        architecture: str = ARCHITECTURE_SF_LITE,
        l2_size: int = DEFAULT_L2_SIZE,
        l3_size: int = DEFAULT_L3_SIZE,
        feature_set: str = FEATURE_SET_HALFKP,
    ) -> None:
        super().__init__()
        if perspective_mode not in ("fixed", "side-to-move"):
            raise ValueError(f"unsupported NNUE perspective mode: {perspective_mode}")
        if architecture not in (ARCHITECTURE_LINEAR, ARCHITECTURE_SF_LITE):
            raise ValueError(f"unsupported NNUE architecture: {architecture}")
        if architecture == ARCHITECTURE_SF_LITE and perspective_mode != "side-to-move":
            raise ValueError("sf-lite requires side-to-move perspective mode")
        if architecture != ARCHITECTURE_SF_LITE and feature_set != FEATURE_SET_HALFKP:
            raise ValueError("non-HalfKP feature sets currently require sf-lite architecture")
        if l2_size <= 0 or l3_size <= 0:
            raise ValueError("dense layer sizes must be positive")
        feature_set = normalize_feature_set(feature_set)
        self.hidden_size = hidden_size
        self.perspective_mode = perspective_mode
        self.architecture = architecture
        self.l2_size = l2_size
        self.l3_size = l3_size
        self.feature_set = feature_set
        self.feature_count = feature_count_for(feature_set)
        if architecture == ARCHITECTURE_SF_LITE:
            self.format_version = FORMAT_VERSION_FEATURE_SET if feature_set != FEATURE_SET_HALFKP else FORMAT_VERSION_SF_LITE
        else:
            self.format_version = (
                FORMAT_VERSION_SIDE_TO_MOVE_PERSPECTIVE
                if perspective_mode == "side-to-move" and feature_set == FEATURE_SET_HALFKP
                else FORMAT_VERSION_FIXED_PERSPECTIVE
            )
            if feature_set != FEATURE_SET_HALFKP:
                self.format_version = FORMAT_VERSION_FEATURE_SET
        self.feature = nn.Embedding(self.feature_count, hidden_size)
        self.hidden_bias = nn.Parameter(torch.zeros(hidden_size))
        if architecture == ARCHITECTURE_SF_LITE:
            self.direct = nn.Linear(hidden_size * 2, 1)
            self.fc0 = nn.Linear(hidden_size * 2, l2_size)
            self.fc1 = nn.Linear(l2_size * 2, l3_size)
            self.fc2 = nn.Linear(l3_size, 1)
        else:
            self.output = nn.Linear(hidden_size * 2, 1)
        self.side_to_move_weight = nn.Parameter(torch.zeros(1))
        nn.init.normal_(self.feature.weight, mean=0.0, std=0.01)
        if architecture == ARCHITECTURE_SF_LITE:
            nn.init.normal_(self.direct.weight, mean=0.0, std=0.005)
            nn.init.zeros_(self.direct.bias)
            nn.init.normal_(self.fc0.weight, mean=0.0, std=0.01)
            nn.init.zeros_(self.fc0.bias)
            nn.init.normal_(self.fc1.weight, mean=0.0, std=0.05)
            nn.init.zeros_(self.fc1.bias)
            nn.init.normal_(self.fc2.weight, mean=0.0, std=0.05)
            nn.init.zeros_(self.fc2.bias)
        else:
            nn.init.normal_(self.output.weight, mean=0.0, std=0.01)
            nn.init.zeros_(self.output.bias)

    def accumulator(self, indices: torch.Tensor, mask: torch.Tensor) -> torch.Tensor:
        embedded = self.feature(indices)
        masked = embedded * mask.unsqueeze(-1)
        return masked.sum(dim=1) + self.hidden_bias

    def forward(
        self,
        white: torch.Tensor,
        white_mask: torch.Tensor,
        black: torch.Tensor,
        black_mask: torch.Tensor,
        side_to_move: torch.Tensor,
    ) -> torch.Tensor:
        white_acc = torch.clamp(self.accumulator(white, white_mask), 0.0, 1.0)
        black_acc = torch.clamp(self.accumulator(black, black_mask), 0.0, 1.0)
        if self.perspective_mode == "side-to-move":
            white_to_move = (side_to_move >= 0).unsqueeze(1)
            stm_acc = torch.where(white_to_move, white_acc, black_acc)
            nstm_acc = torch.where(white_to_move, black_acc, white_acc)
            inputs = torch.cat([stm_acc, nstm_acc], dim=1)
            if self.architecture == ARCHITECTURE_SF_LITE:
                fc0 = torch.clamp(self.fc0(inputs), 0.0, 1.0)
                fc1_input = torch.cat([fc0 * fc0, fc0], dim=1)
                fc1 = torch.clamp(self.fc1(fc1_input), 0.0, 1.0)
                stm_prediction = self.direct(inputs).squeeze(1) + self.fc2(fc1).squeeze(1) + self.side_to_move_weight[0]
            else:
                stm_prediction = self.output(inputs).squeeze(1) + self.side_to_move_weight[0]
            return stm_prediction * side_to_move
        return self.output(torch.cat([white_acc, black_acc], dim=1)).squeeze(1) + side_to_move * self.side_to_move_weight[0]


def make_batch(items: list[tuple[list[int], list[int], float] | tuple[list[int], list[int], int, float]], device: torch.device) -> Batch:
    max_white = max(1, max(len(item[0]) for item in items))
    max_black = max(1, max(len(item[1]) for item in items))
    white = torch.zeros((len(items), max_white), dtype=torch.long)
    black = torch.zeros((len(items), max_black), dtype=torch.long)
    white_mask = torch.zeros((len(items), max_white), dtype=torch.float32)
    black_mask = torch.zeros((len(items), max_black), dtype=torch.float32)
    side_to_move = torch.empty(len(items), dtype=torch.float32)
    target = torch.empty(len(items), dtype=torch.float32)

    for row, item in enumerate(items):
        white_features = item[0]
        black_features = item[1]
        if len(item) == 4:
            side = item[2]
            score = item[3]
        else:
            side = 0
            score = item[2]
        if white_features:
            white[row, : len(white_features)] = torch.tensor(white_features, dtype=torch.long)
            white_mask[row, : len(white_features)] = 1.0
        if black_features:
            black[row, : len(black_features)] = torch.tensor(black_features, dtype=torch.long)
            black_mask[row, : len(black_features)] = 1.0
        side_to_move[row] = side
        target[row] = score

    return Batch(
        white=white.to(device),
        white_mask=white_mask.to(device),
        black=black.to(device),
        black_mask=black_mask.to(device),
        side_to_move=side_to_move.to(device),
        target=target.to(device),
    )


def save_checkpoint(path: Path, model: NnueModel, metadata: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    torch.save(
        {
            "hidden_size": model.hidden_size,
            "format_version": model.format_version,
            "feature_set": model.feature_set,
            "feature_count": model.feature_count,
            "perspective_mode": model.perspective_mode,
            "architecture": model.architecture,
            "l2_size": model.l2_size,
            "l3_size": model.l3_size,
            "state_dict": model.state_dict(),
            "metadata": metadata,
        },
        path,
    )


def load_checkpoint(path: Path, device: torch.device) -> tuple[NnueModel, dict[str, object]]:
    checkpoint = torch.load(path, map_location=device)
    format_version = int(checkpoint.get("format_version", FORMAT_VERSION_FIXED_PERSPECTIVE))
    if format_version not in SUPPORTED_FORMAT_VERSIONS:
        raise ValueError(f"unsupported NNUE checkpoint format {format_version}")
    feature_set = str(checkpoint.get("feature_set", FEATURE_SET_HALFKP))
    if feature_set not in SUPPORTED_FEATURE_SETS:
        raise ValueError(f"unsupported NNUE checkpoint feature set {feature_set}")
    if format_version >= FORMAT_VERSION_SF_LITE:
        architecture = ARCHITECTURE_SF_LITE
        perspective_mode = "side-to-move"
    else:
        architecture = ARCHITECTURE_LINEAR
        perspective_mode = "side-to-move" if format_version >= FORMAT_VERSION_SIDE_TO_MOVE_PERSPECTIVE else "fixed"
    model = NnueModel(
        hidden_size=int(checkpoint["hidden_size"]),
        perspective_mode=perspective_mode,
        architecture=architecture,
        l2_size=int(checkpoint.get("l2_size", DEFAULT_L2_SIZE)),
        l3_size=int(checkpoint.get("l3_size", DEFAULT_L3_SIZE)),
        feature_set=feature_set,
    )
    stored_feature_count = int(checkpoint.get("feature_count", model.feature_count))
    if stored_feature_count != model.feature_count:
        raise ValueError(
            f"checkpoint feature count {stored_feature_count} does not match "
            f"{feature_set} feature count {model.feature_count}"
        )
    state_dict = checkpoint["state_dict"]
    if "side_to_move_weight" not in state_dict:
        state_dict = dict(state_dict)
        state_dict["side_to_move_weight"] = torch.zeros(1)
    model.load_state_dict(state_dict)
    model.to(device)
    model.eval()
    return model, dict(checkpoint.get("metadata", {}))


def load_binary(path: Path) -> BinaryNnue:
    with path.open("rb") as handle:
        magic = handle.read(len(MAGIC))
        if magic != MAGIC:
            raise ValueError(f"invalid NNUE magic in {path}")
        header = handle.read(20)
        if len(header) != 20:
            raise ValueError(f"truncated NNUE header in {path}")
        version, feature_count, hidden_size, accumulator_scale, output_scale = struct.unpack("<IIIII", header)
        if version not in SUPPORTED_FORMAT_VERSIONS:
            raise ValueError(f"unsupported NNUE version {version}")
        feature_set = FEATURE_SET_HALFKP
        if version >= FORMAT_VERSION_FEATURE_SET:
            feature_set_data = handle.read(4)
            if len(feature_set_data) != 4:
                raise ValueError(f"truncated NNUE feature-set header in {path}")
            (feature_set_identifier,) = struct.unpack("<I", feature_set_data)
            feature_set = feature_set_from_id(feature_set_identifier)
        expected_feature_count = feature_count_for(feature_set)
        if feature_count != expected_feature_count:
            raise ValueError(
                f"unsupported feature count {feature_count} for {feature_set}; expected {expected_feature_count}"
            )
        if version >= FORMAT_VERSION_SF_LITE:
            dense_header = handle.read(8)
            if len(dense_header) != 8:
                raise ValueError(f"truncated SF-lite NNUE dense header in {path}")
            l2_size, l3_size = struct.unpack("<II", dense_header)

            def read_float_tensor(count: int, shape: tuple[int, ...]) -> torch.Tensor:
                data = handle.read(count * 4)
                if len(data) != count * 4:
                    raise ValueError(f"truncated SF-lite NNUE float tensor in {path}")
                return torch.frombuffer(bytearray(data), dtype=torch.float32).clone().reshape(shape)

            hidden_bias_float = read_float_tensor(hidden_size, (hidden_size,))
            feature_weights_float = read_float_tensor(feature_count * hidden_size, (feature_count, hidden_size))
            direct_weights = read_float_tensor(hidden_size * 2, (hidden_size * 2,))
            direct_bias_data = handle.read(4)
            if len(direct_bias_data) != 4:
                raise ValueError(f"truncated SF-lite NNUE direct bias in {path}")
            (direct_bias,) = struct.unpack("<f", direct_bias_data)
            fc0_weight = read_float_tensor(l2_size * hidden_size * 2, (l2_size, hidden_size * 2))
            fc0_bias = read_float_tensor(l2_size, (l2_size,))
            fc1_weight = read_float_tensor(l3_size * l2_size * 2, (l3_size, l2_size * 2))
            fc1_bias = read_float_tensor(l3_size, (l3_size,))
            fc2_weight = read_float_tensor(l3_size, (l3_size,))
            fc2_bias_data = handle.read(4)
            side_weight_data = handle.read(4)
            if len(fc2_bias_data) != 4 or len(side_weight_data) != 4:
                raise ValueError(f"truncated SF-lite NNUE output bias in {path}")
            (fc2_bias,) = struct.unpack("<f", fc2_bias_data)
            (side_to_move_weight_float,) = struct.unpack("<f", side_weight_data)
            return BinaryNnue(
                format_version=version,
                feature_count=feature_count,
                feature_set=feature_set,
                hidden_size=hidden_size,
                accumulator_scale=accumulator_scale,
                output_scale=output_scale,
                hidden_bias=torch.empty(0, dtype=torch.int16),
                feature_weights=torch.empty((0, 0), dtype=torch.int16),
                output_weights=torch.empty(0, dtype=torch.int16),
                output_bias=0,
                architecture=ARCHITECTURE_SF_LITE,
                l2_size=l2_size,
                l3_size=l3_size,
                hidden_bias_float=hidden_bias_float,
                feature_weights_float=feature_weights_float,
                direct_weights=direct_weights,
                direct_bias=direct_bias,
                fc0_weight=fc0_weight,
                fc0_bias=fc0_bias,
                fc1_weight=fc1_weight,
                fc1_bias=fc1_bias,
                fc2_weight=fc2_weight,
                fc2_bias=fc2_bias,
                side_to_move_weight_float=side_to_move_weight_float,
            )
        hidden_bias = torch.frombuffer(bytearray(handle.read(hidden_size * 2)), dtype=torch.int16).clone()
        if hidden_bias.numel() != hidden_size:
            raise ValueError(f"truncated hidden bias in {path}")
        feature_weight_count = feature_count * hidden_size
        feature_weights = torch.frombuffer(bytearray(handle.read(feature_weight_count * 2)), dtype=torch.int16).clone()
        if feature_weights.numel() != feature_weight_count:
            raise ValueError(f"truncated feature weights in {path}")
        feature_weights = feature_weights.reshape(feature_count, hidden_size)
        output_weight_count = hidden_size * 2
        output_weights = torch.frombuffer(bytearray(handle.read(output_weight_count * 2)), dtype=torch.int16).clone()
        if output_weights.numel() != output_weight_count:
            raise ValueError(f"truncated output weights in {path}")
        output_bias_data = handle.read(4)
        if len(output_bias_data) != 4:
            raise ValueError(f"truncated output bias in {path}")
        (output_bias,) = struct.unpack("<i", output_bias_data)
        side_to_move_weight = 0
        if version >= 2:
            side_to_move_weight_data = handle.read(4)
            if len(side_to_move_weight_data) != 4:
                raise ValueError(f"truncated side-to-move weight in {path}")
            (side_to_move_weight,) = struct.unpack("<i", side_to_move_weight_data)

    return BinaryNnue(
        format_version=version,
        feature_count=feature_count,
        feature_set=feature_set,
        hidden_size=hidden_size,
        accumulator_scale=accumulator_scale,
        output_scale=output_scale,
        hidden_bias=hidden_bias,
        feature_weights=feature_weights,
        output_weights=output_weights,
        output_bias=output_bias,
        side_to_move_weight=side_to_move_weight,
    )


def evaluate_checkpoint_white_perspective(model: NnueModel, board: chess.Board, device: torch.device) -> float:
    item = (
        active_features(board, chess.WHITE, model.feature_set),
        active_features(board, chess.BLACK, model.feature_set),
        1 if board.turn == chess.WHITE else -1,
        0.0,
    )
    batch = make_batch([item], device)
    with torch.no_grad():
        return float(
            model(batch.white, batch.white_mask, batch.black, batch.black_mask, batch.side_to_move)[0].detach().cpu()
        )


def export_binary(
    path: Path,
    model: NnueModel,
    accumulator_scale: int = DEFAULT_ACCUMULATOR_SCALE,
    output_scale: int = DEFAULT_OUTPUT_SCALE,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if model.architecture == ARCHITECTURE_SF_LITE:
        with torch.no_grad():
            hidden_bias = model.hidden_bias.detach().cpu().to(torch.float32).contiguous()
            feature_weights = model.feature.weight.detach().cpu().to(torch.float32).contiguous()
            direct_weights = model.direct.weight.detach().cpu().squeeze(0).to(torch.float32).contiguous()
            direct_bias = float(model.direct.bias.detach().cpu()[0])
            fc0_weight = model.fc0.weight.detach().cpu().to(torch.float32).contiguous()
            fc0_bias = model.fc0.bias.detach().cpu().to(torch.float32).contiguous()
            fc1_weight = model.fc1.weight.detach().cpu().to(torch.float32).contiguous()
            fc1_bias = model.fc1.bias.detach().cpu().to(torch.float32).contiguous()
            fc2_weight = model.fc2.weight.detach().cpu().squeeze(0).to(torch.float32).contiguous()
            fc2_bias = float(model.fc2.bias.detach().cpu()[0])
            side_to_move_weight = float(model.side_to_move_weight.detach().cpu()[0])
        with path.open("wb") as handle:
            handle.write(MAGIC)
            handle.write(
                struct.pack(
                    "<IIIII",
                    model.format_version,
                    model.feature_count,
                    model.hidden_size,
                    accumulator_scale,
                    output_scale,
                )
            )
            if model.format_version >= FORMAT_VERSION_FEATURE_SET:
                handle.write(struct.pack("<I", feature_set_id(model.feature_set)))
            handle.write(struct.pack("<II", model.l2_size, model.l3_size))
            handle.write(hidden_bias.numpy().tobytes())
            handle.write(feature_weights.numpy().tobytes())
            handle.write(direct_weights.numpy().tobytes())
            handle.write(struct.pack("<f", direct_bias))
            handle.write(fc0_weight.numpy().tobytes())
            handle.write(fc0_bias.numpy().tobytes())
            handle.write(fc1_weight.numpy().tobytes())
            handle.write(fc1_bias.numpy().tobytes())
            handle.write(fc2_weight.numpy().tobytes())
            handle.write(struct.pack("<f", fc2_bias))
            handle.write(struct.pack("<f", side_to_move_weight))
        return

    with torch.no_grad():
        feature_weights = torch.round(model.feature.weight.detach().cpu() * accumulator_scale).clamp(-32768, 32767).to(torch.int16)
        hidden_bias = torch.round(model.hidden_bias.detach().cpu() * accumulator_scale).clamp(-32768, 32767).to(torch.int16)
        output_weights = torch.round(model.output.weight.detach().cpu().squeeze(0) * output_scale / accumulator_scale).clamp(-32768, 32767).to(torch.int16)
        output_bias = int(round(float(model.output.bias.detach().cpu()[0]) * output_scale))
        side_to_move_weight = int(round(float(model.side_to_move_weight.detach().cpu()[0]) * output_scale))

    with path.open("wb") as handle:
        handle.write(MAGIC)
        handle.write(
                struct.pack(
                    "<IIIII",
                    model.format_version,
                    model.feature_count,
                    model.hidden_size,
                    accumulator_scale,
                    output_scale,
                )
            )
        if model.format_version >= FORMAT_VERSION_FEATURE_SET:
            handle.write(struct.pack("<I", feature_set_id(model.feature_set)))
        handle.write(hidden_bias.numpy().tobytes())
        handle.write(feature_weights.contiguous().numpy().tobytes())
        handle.write(output_weights.contiguous().numpy().tobytes())
        handle.write(struct.pack("<i", output_bias))
        handle.write(struct.pack("<i", side_to_move_weight))
