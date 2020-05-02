#include <stack>

#include <libchess/UCIService.h>

#include "eval.h"
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
        : score_(0.0),
          visits_(0),
          move_(move),
          is_terminal_(false),
          parent_(parent),
          visited_children_(0),
          children_(),
          probabilities_() {
    }

    [[nodiscard]] double p(const Position& pos) const noexcept {
        assert(pos.is_legal_move(move_));
        double score = 0.0;

        // MVV-LVA
        if (move_.type() == Move::Type::ENPASSANT) {
            score =
                (libchess::constants::PAWN.value() + 1) * 10 - libchess::constants::PAWN.value();
        } else if (pos.is_capture_move(move_)) {
            const auto victim = pos.piece_on(move_.to_square());
            const auto aggressor = pos.piece_on(move_.from_square());
            score = (victim.value().type().value() + 1) * 10 - aggressor.value().type().value();
        } else {
            score = 0.0;
        }

        assert(score >= 0.0);
        return score;
    }
    [[nodiscard]] double score() const {
        return score_;
    }
    void add_score(const double n) {
        score_ += n;
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
    [[nodiscard]] const std::vector<UCTNode>& children() const noexcept {
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

    [[nodiscard]] double child_probability(const std::size_t idx) const noexcept {
        return probabilities_.at(idx);
    }

    [[nodiscard]] double child_score(const std::size_t idx) const noexcept {
        assert(idx < children_.size());
        assert(idx < probabilities_.size());
        assert(children_.size() == probabilities_.size());
        assert(0.0 <= child_probability(idx) && child_probability(idx) <= 1.0);
        assert(visits() > 0);

        const auto& child = children_.at(idx);

        if (child.visits() == 0) {
            return 30000000.0;
        }

        const double c_puct = 4.0;
        const double Q = child.score() / child.visits();
        const double U =
            c_puct * child_probability(idx) * std::sqrt(visits() - 1) / (child.visits() + 1);
        return Q + U;
    }

    void create_children(const Position& pos, const MoveList& move_list) noexcept {
        children_.reserve(move_list.size());
        probabilities_.reserve(move_list.size());

        double sum = 0.0;
        for (const Move& move : move_list.values()) {
            assert(pos.is_legal_move(move));

            children_.emplace_back(move, this);

            const double score = children_.back().p(pos);
            assert(score >= 0.0);
            sum += score;

            probabilities_.push_back(score);
        }

        // Softmax
        for (auto& prob : probabilities_) {
            if (sum == 0.0) {
                prob = 1.0 / move_list.size();
            } else {
                prob = prob / sum;
            }
            if (prob > 1.0) {
                std::cout << prob << std::endl;
            }
            assert(0.0 <= prob && prob <= 1.0);
        }

#ifndef NDEBUG
        double nsum = 0.0;
        for (auto& prob : probabilities_) {
            nsum += prob;
        }
        assert(std::abs(nsum - 1.0) <= 0.001);
#endif
    }

   private:
    double score_;
    int visits_;
    Move move_;
    bool is_terminal_;
    UCTNode* parent_;
    unsigned visited_children_;
    std::vector<UCTNode> children_;
    std::vector<double> probabilities_;
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

    while (!search_globals.stop()) {
        UCTNode* selected_node = select(pos, &root);
        UCTNode* expanded_node = expand(pos, selected_node);
        double score = rollout(pos, expanded_node);
        backprop(expanded_node, score);

        assert(pos.hash() == original_hash);

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
