from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

import chess
import torch
from torch import nn


FORMAT_VERSION = 1
FEATURE_COUNT = 64 * 10 * 64
DEFAULT_HIDDEN_SIZE = 256
DEFAULT_ACCUMULATOR_SCALE = 256
DEFAULT_OUTPUT_SCALE = 1024
MAGIC = b"CENNUE1\0"

PIECE_SLOT = {
    chess.PAWN: 0,
    chess.KNIGHT: 1,
    chess.BISHOP: 2,
    chess.ROOK: 3,
    chess.QUEEN: 4,
}


def perspective_square(square: chess.Square, perspective: chess.Color) -> chess.Square:
    if perspective == chess.WHITE:
        return square
    return chess.square(chess.square_file(square), 7 - chess.square_rank(square))


def feature_index(
    perspective: chess.Color,
    perspective_king: chess.Square,
    piece: chess.Piece,
    square: chess.Square,
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
    return ((king_square * 10 + piece_slot) * 64) + piece_square


def active_features(board: chess.Board, perspective: chess.Color) -> list[int]:
    king = board.king(perspective)
    if king is None:
        return []
    features: list[int] = []
    for square, piece in board.piece_map().items():
        index = feature_index(perspective, king, piece, square)
        if index is not None:
            features.append(index)
    features.sort()
    return features


@dataclass(frozen=True)
class Batch:
    white: torch.Tensor
    white_mask: torch.Tensor
    black: torch.Tensor
    black_mask: torch.Tensor
    target: torch.Tensor


class NnueModel(nn.Module):
    def __init__(self, hidden_size: int = DEFAULT_HIDDEN_SIZE) -> None:
        super().__init__()
        self.hidden_size = hidden_size
        self.feature = nn.Embedding(FEATURE_COUNT, hidden_size)
        self.hidden_bias = nn.Parameter(torch.zeros(hidden_size))
        self.output = nn.Linear(hidden_size * 2, 1)
        nn.init.normal_(self.feature.weight, mean=0.0, std=0.01)
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
    ) -> torch.Tensor:
        white_acc = torch.clamp(self.accumulator(white, white_mask), 0.0, 1.0)
        black_acc = torch.clamp(self.accumulator(black, black_mask), 0.0, 1.0)
        return self.output(torch.cat([white_acc, black_acc], dim=1)).squeeze(1)


def make_batch(items: list[tuple[list[int], list[int], float]], device: torch.device) -> Batch:
    max_white = max(1, max(len(item[0]) for item in items))
    max_black = max(1, max(len(item[1]) for item in items))
    white = torch.zeros((len(items), max_white), dtype=torch.long)
    black = torch.zeros((len(items), max_black), dtype=torch.long)
    white_mask = torch.zeros((len(items), max_white), dtype=torch.float32)
    black_mask = torch.zeros((len(items), max_black), dtype=torch.float32)
    target = torch.empty(len(items), dtype=torch.float32)

    for row, (white_features, black_features, score) in enumerate(items):
        if white_features:
            white[row, : len(white_features)] = torch.tensor(white_features, dtype=torch.long)
            white_mask[row, : len(white_features)] = 1.0
        if black_features:
            black[row, : len(black_features)] = torch.tensor(black_features, dtype=torch.long)
            black_mask[row, : len(black_features)] = 1.0
        target[row] = score

    return Batch(
        white=white.to(device),
        white_mask=white_mask.to(device),
        black=black.to(device),
        black_mask=black_mask.to(device),
        target=target.to(device),
    )


def save_checkpoint(path: Path, model: NnueModel, metadata: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    torch.save(
        {
            "hidden_size": model.hidden_size,
            "state_dict": model.state_dict(),
            "metadata": metadata,
        },
        path,
    )


def load_checkpoint(path: Path, device: torch.device) -> tuple[NnueModel, dict[str, object]]:
    checkpoint = torch.load(path, map_location=device)
    model = NnueModel(hidden_size=int(checkpoint["hidden_size"]))
    model.load_state_dict(checkpoint["state_dict"])
    model.to(device)
    model.eval()
    return model, dict(checkpoint.get("metadata", {}))


def export_binary(
    path: Path,
    model: NnueModel,
    accumulator_scale: int = DEFAULT_ACCUMULATOR_SCALE,
    output_scale: int = DEFAULT_OUTPUT_SCALE,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with torch.no_grad():
        feature_weights = torch.round(model.feature.weight.detach().cpu() * accumulator_scale).clamp(-32768, 32767).to(torch.int16)
        hidden_bias = torch.round(model.hidden_bias.detach().cpu() * accumulator_scale).clamp(-32768, 32767).to(torch.int16)
        output_weights = torch.round(model.output.weight.detach().cpu().squeeze(0) * output_scale / accumulator_scale).clamp(-32768, 32767).to(torch.int16)
        output_bias = int(round(float(model.output.bias.detach().cpu()[0]) * output_scale))

    with path.open("wb") as handle:
        handle.write(MAGIC)
        handle.write(
            struct.pack(
                "<IIIII",
                FORMAT_VERSION,
                FEATURE_COUNT,
                model.hidden_size,
                accumulator_scale,
                output_scale,
            )
        )
        handle.write(hidden_bias.numpy().tobytes())
        handle.write(feature_weights.contiguous().numpy().tobytes())
        handle.write(output_weights.contiguous().numpy().tobytes())
        handle.write(struct.pack("<i", output_bias))
