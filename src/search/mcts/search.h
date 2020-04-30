#ifndef MEGUMAX_SEARCH_H
#define MEGUMAX_SEARCH_H

#include "search_globals.h"

namespace megumax {

std::optional<libchess::Move> search(libchess::Position& pos, SearchGlobals& search_globals);

}  // namespace megumax

#endif  // MEGUMAX_SEARCH_H
