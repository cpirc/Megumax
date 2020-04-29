#include "search.h"
#include "rng_service.h"

using libchess::Move;
using libchess::MoveList;
using libchess::Position;

namespace megumax {

std::optional<Move> search(Position& pos, SearchGlobals& search_globals) {
    auto start_time = curr_time();
    search_globals.stop_flag(false);
    search_globals.side_to_move(pos.side_to_move());
    search_globals.reset_nodes();
    search_globals.start_time(start_time);

    if (search_globals.stop()) {
        return std::nullopt;
    }

    MoveList move_list = pos.legal_move_list();
    if (move_list.empty()) {
        return std::nullopt;
    }

    RNGService* rng_service = RNGService::singleton();
    std::uint32_t rand_idx = rng_service->rand_uint32(0, move_list.size() - 1);
    return move_list.values().at(rand_idx);
}

}  // namespace megumax
