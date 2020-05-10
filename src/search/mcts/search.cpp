#include <optional>
#include <stack>
#include <string>

#include <libchess/UCIService.h>

#include "eval.h"
#include "rng_service.h"
#include "search.h"
#include "uct_node.h"

using libchess::Color;
using libchess::Move;
using libchess::MoveList;
using libchess::Position;
using libchess::UCIService;

namespace megumax {

void forward_position(Position& pos, UCTNode* node) {
    std::stack<Move> move_stack;
    UCTNode* iter = node;
    while (iter->parent() != nullptr) {
        move_stack.push(iter->move());
        iter = iter->parent();
    }
    while (!move_stack.empty()) {
        pos.make_move(move_stack.top());
        move_stack.pop();
    }
}

void rewind_position(Position& pos, int times) {
    while (times > 0) {
        --times;
        pos.unmake_move();
    }
}

unsigned select_most_visited_child_index(std::vector<UCTNode>& children) {
    unsigned most_visited_node_index = 0;
    for (unsigned i = 1; i < children.size(); ++i) {
        if (children.at(i).visits() > children.at(most_visited_node_index).visits()) {
            most_visited_node_index = i;
        }
    }
    return most_visited_node_index;
}

unsigned select_best_child_index(const UCTNode* node) {
    unsigned best_node_index = 0;
    for (unsigned i = 1; i < node->children().size(); ++i) {
        if (node->child_score(i) > node->child_score(best_node_index)) {
            best_node_index = i;
        }
    }
    return best_node_index;
}

UCTNode* select(Position& pos, UCTNode* node) {
    std::vector<UCTNode>& children = node->children();
    if (children.empty() || node->visited_children() < children.size()) {
        return node;
    }

    unsigned best_child_index = select_best_child_index(node);
    assert(pos.is_legal_move(children.at(best_child_index).move()));
    pos.make_move(children.at(best_child_index).move());
    return select(pos, &children.at(best_child_index));
}

UCTNode* expand(Position& pos, UCTNode* selected_node) {
    std::vector<UCTNode>& children = selected_node->children();
    if (children.empty()) {
        if (selected_node->is_terminal()) {
            return selected_node;
        }

        MoveList move_list = pos.legal_move_list();
        if (move_list.empty()) {
            selected_node->is_terminal(true);
            return selected_node;
        }

        selected_node->create_children(pos, move_list);

        return selected_node;
    }

    unsigned next_child_index = selected_node->visited_children();
    selected_node->increment_visited_children();
    UCTNode* next_child = &selected_node->children().at(next_child_index);
    assert(pos.is_legal_move(next_child->move()));
    pos.make_move(next_child->move());
    return next_child;
}

double sigmoid(double score, double k = 1.13) noexcept {
    return 1.0 / (1.0 + std::pow(10.0, -k * score / 400.0));
}

double rollout(Position& forwarded_position, UCTNode* expanded_node) {
    double score;

    switch (forwarded_position.game_state()) {
        case Position::GameState::THREEFOLD_REPETITION:
        case Position::GameState::FIFTY_MOVES:
        case Position::GameState::STALEMATE:
            score = 0.5;
            break;
        case Position::GameState::CHECKMATE:
            score = 0.0;
            break;
        case Position::GameState::IN_PROGRESS:
            score = sigmoid(0.1 * eval(forwarded_position));
            break;
        default:
            abort();
    }

    rewind_position(forwarded_position, expanded_node->depth());
    return 1.0 - score;
}

void backprop(UCTNode* rolled_out_node, const double score) {
    if (rolled_out_node == nullptr) {
        return;
    }
    rolled_out_node->increment_visits();
    rolled_out_node->add_score(score);
    backprop(rolled_out_node->parent(), 1.0 - score);
}

MoveList get_pv(UCTNode* node, const int max_length = 8) {
    MoveList move_list;
    int ply = 0;
    while (!node->children().empty() && ply < max_length) {
        const auto idx = select_most_visited_child_index(node->children());
        move_list.add(node->children().at(idx).move());
        node = &node->children().at(idx);
        ply++;
    }
    return move_list;
}

bool legal_pv(Position& pos, MoveList&& move_list) {
    int ply = 0;

    for (const auto& move : move_list) {
        if (pos.is_legal_move(move)) {
            pos.make_move(move);
            ply++;
        } else {
            break;
        }
    }

    for (int i = 0; i < ply; ++i) {
        pos.unmake_move();
    }

    return ply == move_list.size();
}

void stats(Position& pos, UCTNode* node) {
    forward_position(pos, node);

    pos.display();
    UCTNode* parent = node->parent();
    std::cout << "depth: " << node->depth() << "\n"
              << "visits: " << node->visits() << "\n"
              << "score: " << node->score() << "\n"
              << "P: " << node->p(pos) << "\n"
              << "Q: " << node->score() / node->visits() << "\n";
    if (parent != nullptr) {
        auto parent_relative_idx = node - parent->children().data();
        std::cout << "U: " << parent->child_probability(parent_relative_idx) << "\n";
    }
    bool is_expanded = (!node->is_terminal() && !node->children().empty());
    std::cout << "expanded: " << std::to_string(is_expanded) << "\n";

    rewind_position(pos, node->depth());
}

std::optional<Move> search(Position& pos, SearchGlobals& search_globals) {
    search_globals.stop_flag(false);
    search_globals.side_to_move(pos.side_to_move());
    search_globals.reset_nodes();
    auto start_time = curr_time();
    auto last_info_time = start_time;
    search_globals.start_time(start_time);

    if (search_globals.stop()) {
        return std::nullopt;
    }

    UCTNode root{Move{0}, nullptr};

#ifndef NDEBUG
    const auto original_hash = pos.hash();
#endif

    int debug_steps = 0;
    while (!search_globals.stop()) {
        do {
            if (!search_globals.debug()) {
                break;
            }
            if (debug_steps > 0) {
                --debug_steps;
                if (debug_steps > 0) {
                    break;
                }
            }
            std::string line;
            UCTNode* selected_node = &root;
            std::cout << "Debug mode activated, selected node is root.\n";
            while (true) {
                stats(pos, selected_node);
                std::getline(std::cin, line);
                if (line == "moves" || line == "children" || line == "ls") {
                    if (selected_node->is_terminal()) {
                        std::cout << "Selected node is terminal!\n";
                        continue;
                    } else if (selected_node->children().empty()) {
                        std::cout << "Selected node is not yet expanded!\n";
                        continue;
                    }
                    for (unsigned i = 0; i < selected_node->children().size(); ++i) {
                        UCTNode& child = selected_node->children().at(i);
                        // clang-format off
                        std::cout << "move " << child.move().to_str()
                                  << " visits " << child.visits()
                                  << " score " << child.score()
                                  << " prior_probability " << selected_node->child_probability(i)
                                  << "\n";
                        // clang-format on
                    }
                } else if (line.find("child", 0) != std::string::npos) {
                    if (selected_node->is_terminal()) {
                        std::cout << "Selected node is terminal!\n";
                        continue;
                    } else if (selected_node->children().empty()) {
                        std::cout << "Selected node is not yet expanded!\n";
                        continue;
                    }
                    auto move_start_pos = line.find(' ');
                    if (move_start_pos == std::string::npos) {
                        std::cout << line << " is not a valid move command!\n";
                        continue;
                    }
                    std::string move_str = line.substr(move_start_pos + 1);
                    std::optional<Move> move = Move::from(move_str);
                    if (!move) {
                        std::cout << move_str << " is not a valid move format!\n";
                        continue;
                    }
                    auto& children = selected_node->children();
                    int found_idx = -1;
                    for (unsigned i = 0; i < children.size(); ++i) {
                        if (children.at(i).move() == *move) {
                            found_idx = (int)i;
                            break;
                        }
                    }
                    if (found_idx == -1) {
                        std::cout << move_str << " is not a legal move in the current position!\n";
                        break;
                    }
                    selected_node = children.data() + found_idx;
                } else if (line == "parent") {
                    UCTNode* parent = selected_node->parent();
                    if (parent == nullptr) {
                        std::cout << "Selected node is root!\n";
                        continue;
                    }
                    selected_node = parent;
                } else if (line == "step" || line == "s" ||
                           line.find("steps") != std::string::npos) {
                    auto debug_steps_pos = line.find(' ');
                    if (debug_steps_pos == std::string::npos) {
                        debug_steps = 1;
                    } else {
                        std::string debug_steps_str = line.substr(debug_steps_pos + 1);
                        debug_steps = std::stoi(debug_steps_str);
                    }
                    break;
                } else if (line == "ndebug" || line == "quit" || line == "stop") {
                    std::lock_guard<std::mutex> debug_lock(search_globals.debug_mutex);
                    search_globals.debug(false);
                    search_globals.debug_cv.notify_all();
                    break;
                }
            }
        } while (false);

        UCTNode* selected_node = select(pos, &root);
        UCTNode* expanded_node = expand(pos, selected_node);
        double score = rollout(pos, expanded_node);
        backprop(expanded_node, score);

        assert(pos.hash() == original_hash);
        assert(legal_pv(pos, get_pv(&root)));

        search_globals.increment_nodes();

        if (search_globals.nodes() % 1000 == 0) {
            auto now = curr_time();
            auto time_diff = now - start_time;
            std::uint64_t time_since_last_info = (now - last_info_time).count();
            if (time_since_last_info >= 1000) {
                const auto pv = get_pv(&root);
                std::uint64_t time_ms = time_diff.count();
                std::uint64_t nodes = search_globals.nodes();
                std::cout << "info";
                // std::cout << " score cp " << root.value(pos);
                std::cout << " nodes " << nodes;
                std::cout << " time " << time_ms;
                std::cout << " nps " << (time_ms ? (nodes * 1000 / time_ms) : nodes);
                if (!pv.empty()) {
                    std::cout << " pv";
                    for (const auto& move : pv.values()) {
                        std::cout << " " << move.to_str();
                    }
                }
                std::cout << "\n";
                last_info_time = now;
            }
        }
    }

    unsigned best_child_index = select_most_visited_child_index(root.children());
    return {root.children().at(best_child_index).move()};
}

}  // namespace megumax
