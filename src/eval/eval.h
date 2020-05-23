#ifndef MEGUMAX_EVAL_EVAL_H
#define MEGUMAX_EVAL_EVAL_H

#include <array>

#include <libchess/Position.h>

namespace megumax {

constexpr static std::array<int, 6> piece_values{100, 300, 325, 500, 900, 100000};

int eval(const libchess::Position& pos);

}  // namespace megumax

#endif  // MEGUMAX_EVAL_EVAL_H
