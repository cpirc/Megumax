#include "eval.h"
#include "pst.h"

namespace megumax {

constexpr int piece_values[] = {100, 300, 325, 500, 900, 100000};

int count_piece(const libchess::Position& pos, const libchess::PieceType pt) {
    return pos.piece_type_bb(pt, libchess::constants::WHITE).popcount() +
           pos.piece_type_bb(pt, libchess::constants::BLACK).popcount();
}

int phase(const libchess::Position& pos) {
    int phase = 24;
    phase -= count_piece(pos, libchess::constants::KNIGHT);
    phase -= count_piece(pos, libchess::constants::BISHOP);
    phase -= 2 * count_piece(pos, libchess::constants::ROOK);
    phase -= 4 * count_piece(pos, libchess::constants::QUEEN);
    return (phase * 256 + 12) / 24;
}

int eval(const libchess::Position& pos) {
    int score = 0;
    int mg_score = 0;
    int eg_score = 0;
    auto occupancy = pos.occupancy_bb();

    while (occupancy) {
        const auto sq = occupancy.forward_bitscan();
        occupancy.forward_popbit();

        const auto piece = pos.piece_on(sq);
        const auto colour = piece->color();
        const int mul = colour == libchess::constants::WHITE ? 1 : -1;

        // Material
        score += piece_values[piece.value().type()] * mul;

        // PST
        if (colour == libchess::constants::WHITE) {
            mg_score += pst_mg(piece.value().type(), sq);
            eg_score += pst_eg(piece.value().type(), sq);
        } else {
            mg_score -= pst_mg(piece.value().type(), sq.flipped());
            eg_score -= pst_eg(piece.value().type(), sq.flipped());
        }
    }

    // Phase
    const int p = phase(pos);
    score += ((mg_score * (256 - p)) + (eg_score * p)) / 256;

    // Return from side to move's pov
    if (pos.side_to_move() != libchess::constants::WHITE) {
        score = -score;
    }

    return score;
}

}  // namespace megumax
