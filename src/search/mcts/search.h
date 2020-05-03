#ifndef MEGUMAX_MCTS_SEARCH_H
#define MEGUMAX_MCTS_SEARCH_H

#include "search_globals.h"

namespace megumax {

std::optional<libchess::Move> search(libchess::Position& pos, SearchGlobals& search_globals);

}  // namespace megumax

#endif  // MEGUMAX_MCTS_SEARCH_H
