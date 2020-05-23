#ifndef MEGUMAX_EVAL_EVAL_H
#define MEGUMAX_EVAL_EVAL_H

#include <libchess/Position.h>

namespace megumax {

int eval(const libchess::Position& pos);

}  // namespace megumax

#endif  // MEGUMAX_EVAL_EVAL_H
