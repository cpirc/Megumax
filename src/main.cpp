#include "libchess/Position.h"
#include "libchess/UCIService.h"

#include "search.h"

using libchess::Move;
using libchess::Position;
using libchess::UCIGoParameters;
using libchess::UCIInfoParameters;
using libchess::UCIPositionParameters;
using libchess::UCIService;

using megumax::SearchGlobals;

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf);

    Position position{libchess::constants::STARTPOS_FEN};
    SearchGlobals search_globals = SearchGlobals::new_search_globals();
    auto position_handler = [&position](const UCIPositionParameters& position_parameters) {
        position = Position{position_parameters.fen()};
        if (!position_parameters.move_list()) {
            return;
        }
        for (auto& move_str : position_parameters.move_list()->move_list()) {
            position.make_move(*Move::from(move_str));
        }
    };
    auto go_handler = [&position, &search_globals](const UCIGoParameters& go_parameters) {
        search_globals.go_parameters(go_parameters);
        auto best_move = megumax::search(position, search_globals);
        if (best_move) {
            UCIService::bestmove(best_move->to_str());
        } else {
            UCIService::bestmove("0000");
        }
    };
    auto stop_handler = [&search_globals]() { search_globals.stop_flag(true); };
    auto display_handler = [&position](const std::istringstream&) { position.display(); };

    UCIService uci_service{"Megumax", "##chessprogramming Freenode IRC"};
    uci_service.register_position_handler(position_handler);
    uci_service.register_go_handler(go_handler);
    uci_service.register_stop_handler(stop_handler);
    uci_service.register_handler("d", display_handler);

    std::string line;
    while (true) {
        std::getline(std::cin, line);
        if (line == "uci") {
            uci_service.run();
            break;
        } else {
            std::cout << "Supported Protocols: uci\n";
        }
    }

    return 0;
}
