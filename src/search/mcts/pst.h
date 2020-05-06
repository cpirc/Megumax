#ifndef MEGUMAX_SEARCH_MCTS_PST_H
#define MEGUMAX_SEARCH_MCTS_PST_H

#include <optional>

#include <libchess/PieceType.h>
#include <libchess/Square.h>

namespace megumax {

int pst_mg(const libchess::PieceType pt, const libchess::Square sq);
int pst_eg(const libchess::PieceType pt, const libchess::Square sq);

}  // namespace megumax

#endif
