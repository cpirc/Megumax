#include "eval.h"
#include "masks.h"
#include "pst.h"

using libchess::Bitboard;
using libchess::Color;
using libchess::PieceType;
using libchess::Position;
using libchess::Square;

namespace constants = libchess::constants;

namespace megumax {

constexpr int MG = 0;
constexpr int EG = 1;

constexpr std::array<std::array<std::array<int, 7>, 2>, 2> passed_pawn_values{{
    {{{0, 5, 10, 20, 30, 50, 100}, {0, 5, 10, 20, 40, 70, 120}}},
    {{{100, 50, 30, 20, 10, 5, 0}, {120, 70, 40, 20, 10, 5, 0}}},
}};

int phase(const Position& pos) {
    int phase = 24;
    phase -= pos.piece_type_bb(constants::KNIGHT).popcount();
    phase -= pos.piece_type_bb(constants::BISHOP).popcount();
    phase -= 2 * pos.piece_type_bb(constants::ROOK).popcount();
    phase -= 4 * pos.piece_type_bb(constants::QUEEN).popcount();
    return (phase * 256 + 12) / 24;
}

int eval(const Position& pos) {
    int score = 0;
    int mg_score = 0;
    int eg_score = 0;

    Bitboard pawn_bb[2];

    for (Color color : constants::COLORS) {
        pawn_bb[color] = pos.piece_type_bb(constants::PAWN, color);

        for (PieceType piece_type : constants::PIECE_TYPES) {
            Bitboard piece_bb = pos.piece_type_bb(piece_type, color);

            // Material
            score += piece_values[piece_type.value()] * piece_bb.popcount();

            while (piece_bb) {
                Square piece_sq = piece_bb.forward_bitscan();
                piece_bb.forward_popbit();

                // PST
                mg_score += pst_mg(piece_type, color, piece_sq);
                eg_score += pst_eg(piece_type, color, piece_sq);
            }
        }

        score = -score;
        mg_score = -mg_score;
        eg_score = -eg_score;
    }

    for (Color color : constants::COLORS) {
        Bitboard bb = pawn_bb[color];
        while (bb) {
            Square sq = bb.forward_bitscan();
            bb.forward_popbit();

            bool is_passed = !(passed_pawn_mask(sq, color) & pawn_bb[!color]);
            if (is_passed) {
                mg_score += passed_pawn_values[color][MG][sq.rank()];
                eg_score += passed_pawn_values[color][EG][sq.rank()];
            }
        }

        mg_score = -mg_score;
        eg_score = -eg_score;
    }

    // Phase
    const int p = phase(pos);
    score += ((mg_score * (256 - p)) + (eg_score * p)) / 256;

    // Return from side to move's pov
    if (pos.side_to_move() != constants::WHITE) {
        score = -score;
    }

    return score;
}

}  // namespace megumax
