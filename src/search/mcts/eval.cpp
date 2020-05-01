#include "eval.h"

namespace megumax {

constexpr int piece_values[] = {100, 300, 325, 500, 900, 100000};

int eval(const libchess::Position& pos) {
    int score = 0;
    auto occupancy = pos.occupancy_bb();

    while (occupancy) {
        const auto sq = occupancy.forward_bitscan();
        occupancy.forward_popbit();

        const auto piece = pos.piece_on(sq);
        const auto colour = piece->color();
        const int mul = colour == libchess::constants::WHITE ? 1 : -1;

        // Material
        score += piece_values[piece.value().type()] * mul;
    }

    // Return from side to move's pov
    if (pos.side_to_move() != libchess::constants::WHITE) {
        score = -score;
    }

    return score;
}

}  // namespace megumax
