#include <stack>
#include <utility>

#include <libchess/UCIService.h>

#include "rng_service.h"
#include "search.h"

using libchess::Color;
using libchess::Move;
using libchess::MoveList;
using libchess::Position;
using libchess::UCIService;

namespace megumax {

class UCTNode {
   public:
    UCTNode(Move move, UCTNode* parent)
        : wins_(0),
          visits_(0),
          move_(move),
          is_terminal_(false),
          parent_(parent),
          visited_children_(0),
          children_() {
    }

    [[nodiscard]] double value() const {
        if (visits_ == 0) {
            return 30000000.0;
        }
        double win_ratio = (wins_ / (double)visits_);
        if (parent_ == nullptr) {
            return win_ratio;
        }
        return win_ratio + std::sqrt(2.0 * std::log(parent_->visits()) / visits_);
    }
    void increment_wins() {
        ++wins_;
    }
    [[nodiscard]] int visits() const {
        return visits_;
    }
    void increment_visits() {
        ++visits_;
    }
    [[nodiscard]] const Move& move() const {
        return move_;
    }
    [[nodiscard]] bool is_terminal() const {
        return is_terminal_;
    }
    void is_terminal(bool is_terminal) {
        is_terminal_ = is_terminal;
    }
    [[nodiscard]] UCTNode* parent() const {
        return parent_;
    }
    [[nodiscard]] unsigned visited_children() const {
        return visited_children_;
    }
    void increment_visited_children() {
        ++visited_children_;
    }
    [[nodiscard]] std::vector<UCTNode>& children() {
        return children_;
    }

    [[nodiscard]] int depth() const {
        const UCTNode* iter = this;
        int ply = 0;
        while (iter->parent_ != nullptr) {
            iter = iter->parent();
            ++ply;
        }
        return ply;
    }

   private:
    int wins_;
    int visits_;
    Move move_;
    bool is_terminal_;
    UCTNode* parent_;
    unsigned visited_children_;
    std::vector<UCTNode> children_;
};

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

unsigned select_best_child_index(std::vector<UCTNode>& children) {
    unsigned best_node_index = 0;
    for (unsigned i = 1; i < children.size(); ++i) {
        if (children.at(i).value() > children.at(best_node_index).value()) {
            best_node_index = i;
        }
    }
    return best_node_index;
}

UCTNode* select(UCTNode* node) {
    std::vector<UCTNode>& children = node->children();
    if (children.empty() || node->visited_children() < children.size()) {
        return node;
    }

    unsigned best_child_index = select_best_child_index(node->children());
    return select(&children.at(best_child_index));
}

UCTNode* expand(Position& pos, UCTNode* selected_node) {
    forward_position(pos, selected_node);

    std::vector<UCTNode>& children = selected_node->children();
    if (children.empty() || selected_node->visited_children() < children.size()) {
        if (selected_node->is_terminal()) {
            return selected_node;
        }

        MoveList move_list = pos.legal_move_list();
        if (move_list.empty()) {
            selected_node->is_terminal(true);
            return selected_node;
        }

        children.reserve(move_list.size());
        for (Move move : move_list) {
            children.emplace_back(move, selected_node);
        }
        return selected_node;
    }

    unsigned next_child_index = selected_node->visited_children();
    selected_node->increment_visited_children();
    return &selected_node->children().at(next_child_index);
}

int rollout(Position& forwarded_position, UCTNode* expanded_node) {
    RNGService* rng_service = RNGService::singleton();
    int rollout_ply = 0;
    MoveList move_list = forwarded_position.legal_move_list();
    Color side_to_move = forwarded_position.side_to_move();
    while (!move_list.empty()) {
        if (forwarded_position.halfmoves() >= 100 || forwarded_position.is_repeat(2)) {
            break;
        }
        std::uint32_t random_move_index = rng_service->rand_uint32(0, move_list.size() - 1);
        forwarded_position.make_move(move_list.values().at(random_move_index));
        ++rollout_ply;
        move_list = forwarded_position.legal_move_list();
    }
    std::optional<Color> loser = forwarded_position.in_check()
                                     ? std::optional{forwarded_position.side_to_move()}
                                     : std::nullopt;
    rewind_position(forwarded_position, rollout_ply + expanded_node->depth());
    if (loser) {
        return side_to_move == *loser ? -1 : 1;
    }
    return 0;
}

void backprop(UCTNode* rolled_out_node, int score) {
    if (rolled_out_node == nullptr) {
        return;
    }
    rolled_out_node->increment_visits();
    if (score == 1) {
        rolled_out_node->increment_wins();
    }
    backprop(rolled_out_node->parent(), -1 * score);
}

std::optional<Move> search(Position& pos, SearchGlobals& search_globals) {
    search_globals.stop_flag(false);
    search_globals.side_to_move(pos.side_to_move());
    search_globals.reset_nodes();
    search_globals.start_time(curr_time());

    if (search_globals.stop()) {
        return std::nullopt;
    }

    UCTNode root{Move{0}, nullptr};

    while (!search_globals.stop()) {
        UCTNode* selected_node = select(&root);
        UCTNode* expanded_node = expand(pos, selected_node);
        int score = rollout(pos, expanded_node);
        backprop(expanded_node, score);

        search_globals.increment_nodes();

        if (search_globals.nodes() % 1000 == 0) {
            // clang-format off
            std::cout << "info"
                      << " score cp " << root.value()
                      << " nodes " << search_globals.nodes()
                      << "\n";
            // clang-format on
        }
    }

    unsigned best_child_index = select_most_visited_child_index(root.children());
    return {root.children().at(best_child_index).move()};
}

}  // namespace megumax