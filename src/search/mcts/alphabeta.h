#ifndef MEGUMAX_MCTS_ALPHABETA_H
#define MEGUMAX_MCTS_ALPHABETA_H

#include "eval.h"
#include "libchess/Position.h"

namespace megumax {

int alphabeta(libchess::Position& pos, int alpha, const int beta, int depth) {
    if (depth == 0) {
        return eval(pos);
    }

    auto move_list = pos.legal_move_list();
    int best_score = std::numeric_limits<int>::min();
    const bool in_check = pos.in_check();

    // Checkmate or stalemate
    if (move_list.size() == 0) {
        if (in_check) {
            return -9999999;
        } else {
            return 0;
        }
    }

    for (const auto& move : move_list.values()) {
        pos.make_move(move);
        const int score = -alphabeta(pos, -beta, -alpha, depth - 1);
        pos.unmake_move();

        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                alpha = score;
                if (alpha >= beta) {
                    break;
                }
            }
        }
    }

    return alpha;
}

}  // namespace megumax

#endif
