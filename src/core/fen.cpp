#include "chess/core/fen.h"

#include <charconv>
#include <system_error>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace chess {

namespace {

std::vector<std::string> split_fields(std::string_view value) {
    std::istringstream stream{std::string(value)};
    std::vector<std::string> fields;
    std::string field;
    while (stream >> field) {
        fields.push_back(field);
    }
    return fields;
}

int parse_int_field(const std::string& field, const char* field_name) {
    int value = 0;
    const char* begin = field.data();
    const char* end = field.data() + field.size();
    const auto [ptr, error] = std::from_chars(begin, end, value);
    if (error != std::errc{} || ptr != end) {
        throw std::invalid_argument(std::string("FEN ") + field_name + " field is invalid");
    }
    return value;
}

}  // namespace

Board board_from_fen(std::string_view fen) {
    const auto fields = split_fields(fen);
    if (fields.size() != 6) {
        throw std::invalid_argument("FEN must contain exactly six fields");
    }

    Board board;
    board.clear();

    int rank = 7;
    int file = 0;
    int white_kings = 0;
    int black_kings = 0;

    for (const char value : fields[0]) {
        if (value == '/') {
            if (file != 8) {
                throw std::invalid_argument("FEN rank does not contain eight files");
            }
            --rank;
            file = 0;
            continue;
        }

        if (value >= '1' && value <= '8') {
            file += value - '0';
            if (file > 8) {
                throw std::invalid_argument("FEN rank contains too many files");
            }
            continue;
        }

        const Piece piece = char_to_piece(value);
        if (piece == Piece::None) {
            throw std::invalid_argument("FEN contains invalid piece character");
        }
        if (rank < 0 || file >= 8) {
            throw std::invalid_argument("FEN board contains too many squares");
        }
        board.set_piece(make_square(file, rank), piece);
        if (piece == Piece::WhiteKing) {
            ++white_kings;
        } else if (piece == Piece::BlackKing) {
            ++black_kings;
        }
        ++file;
    }

    if (rank != 0 || file != 8) {
        throw std::invalid_argument("FEN board does not contain eight ranks");
    }
    if (white_kings != 1 || black_kings != 1) {
        throw std::invalid_argument("FEN must contain exactly one king per side");
    }

    if (fields[1] == "w") {
        board.set_side_to_move(Color::White);
    } else if (fields[1] == "b") {
        board.set_side_to_move(Color::Black);
    } else {
        throw std::invalid_argument("FEN side-to-move field must be w or b");
    }

    std::uint8_t rights = 0;
    if (fields[2] != "-") {
        for (const char value : fields[2]) {
            std::uint8_t right = 0;
            switch (value) {
                case 'K':
                    right = WhiteKingSide;
                    break;
                case 'Q':
                    right = WhiteQueenSide;
                    break;
                case 'k':
                    right = BlackKingSide;
                    break;
                case 'q':
                    right = BlackQueenSide;
                    break;
                default:
                    throw std::invalid_argument("FEN castling rights field is invalid");
            }
            if ((rights & right) != 0) {
                throw std::invalid_argument("FEN castling rights field contains duplicate rights");
            }
            rights |= right;
        }
    }
    board.set_castling_rights(rights);

    if (fields[3] == "-") {
        board.set_en_passant_square(kNoSquare);
    } else {
        const Square ep = square_from_string(fields[3]);
        if (!is_valid_square(ep) || (rank_of(ep) != 2 && rank_of(ep) != 5)) {
            throw std::invalid_argument("FEN en-passant square is invalid");
        }
        board.set_en_passant_square(ep);
    }

    board.set_halfmove_clock(parse_int_field(fields[4], "halfmove clock"));
    board.set_fullmove_number(parse_int_field(fields[5], "fullmove number"));
    if (board.halfmove_clock() < 0 || board.fullmove_number() <= 0) {
        throw std::invalid_argument("FEN move counters are invalid");
    }

    return board;
}

std::string Board::to_fen() const {
    std::ostringstream out;

    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            const Piece piece = piece_at(make_square(file, rank));
            if (piece == Piece::None) {
                ++empty;
                continue;
            }
            if (empty > 0) {
                out << empty;
                empty = 0;
            }
            out << piece_to_char(piece);
        }
        if (empty > 0) {
            out << empty;
        }
        if (rank > 0) {
            out << '/';
        }
    }

    out << ' ' << (side_to_move_ == Color::White ? 'w' : 'b') << ' ';
    if (castling_rights_ == 0) {
        out << '-';
    } else {
        if ((castling_rights_ & WhiteKingSide) != 0) {
            out << 'K';
        }
        if ((castling_rights_ & WhiteQueenSide) != 0) {
            out << 'Q';
        }
        if ((castling_rights_ & BlackKingSide) != 0) {
            out << 'k';
        }
        if ((castling_rights_ & BlackQueenSide) != 0) {
            out << 'q';
        }
    }

    out << ' ';
    if (en_passant_square_ == kNoSquare) {
        out << '-';
    } else {
        out << square_to_string(en_passant_square_);
    }

    out << ' ' << halfmove_clock_ << ' ' << fullmove_number_;
    return out.str();
}

}  // namespace chess
