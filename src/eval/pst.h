#ifndef MEGUMAX_EVAL_PST_H
#define MEGUMAX_EVAL_PST_H

#include <optional>

#include <libchess/PieceType.h>
#include <libchess/Square.h>

namespace megumax {

int pst_mg(libchess::PieceType pt, libchess::Square sq);
int pst_eg(libchess::PieceType pt, libchess::Square sq);

}  // namespace megumax

#endif  // MEGUMAX_EVAL_PST_H
