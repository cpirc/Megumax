#include "eval.h"
#include "pst.h"

using libchess::Bitboard;
using libchess::Color;
using libchess::PieceType;
using libchess::Position;
using libchess::Square;

namespace constants = libchess::constants;

namespace megumax {

constexpr int piece_values[] = {100, 300, 325, 500, 900, 100000};

int count_piece(const Position& pos, const PieceType pt) {
    return pos.piece_type_bb(pt, constants::WHITE).popcount() +
           pos.piece_type_bb(pt, constants::BLACK).popcount();
}

int phase(const Position& pos) {
    int phase = 24;
    phase -= count_piece(pos, constants::KNIGHT);
    phase -= count_piece(pos, constants::BISHOP);
    phase -= 2 * count_piece(pos, constants::ROOK);
    phase -= 4 * count_piece(pos, constants::QUEEN);
    return (phase * 256 + 12) / 24;
}

int eval(const Position& pos) {
    int score = 0;
    int mg_score = 0;
    int eg_score = 0;

    for (Color color : constants::COLORS) {
        for (PieceType piece_type : constants::PIECE_TYPES) {
            Bitboard piece_bb = pos.piece_type_bb(piece_type, color);

            // Material
            score += piece_values[piece_type.value()] * piece_bb.popcount();

            while (piece_bb) {
                Square piece_sq = piece_bb.forward_bitscan();
                piece_bb.forward_popbit();

                // PST
                if (color == constants::WHITE) {
                    mg_score += pst_mg(piece_type, piece_sq);
                    eg_score += pst_eg(piece_type, piece_sq);
                } else {
                    mg_score += pst_mg(piece_type, piece_sq.flipped());
                    eg_score += pst_eg(piece_type, piece_sq.flipped());
                }
            }
        }

        score = -score;
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
