#ifndef MEGUMAX_MCTS_UCT_NODE_H
#define MEGUMAX_MCTS_UCT_NODE_H

#include <cassert>
#include <cmath>

#include "libchess/Position.h"

namespace megumax {

class UCTNode {
   public:
    UCTNode(libchess::Move move, UCTNode* parent);

    [[nodiscard]] double p(const libchess::Position& pos) const noexcept;
    [[nodiscard]] double score() const;
    void add_score(const double n);
    [[nodiscard]] int visits() const;
    void increment_visits();
    [[nodiscard]] const libchess::Move& move() const;
    [[nodiscard]] bool is_terminal() const;
    void is_terminal(bool is_terminal);
    [[nodiscard]] UCTNode* parent() const;
    [[nodiscard]] unsigned visited_children() const;
    void increment_visited_children();
    [[nodiscard]] std::vector<UCTNode>& children();
    [[nodiscard]] const std::vector<UCTNode>& children() const noexcept;

    [[nodiscard]] int depth() const;

    [[nodiscard]] double child_probability(const std::size_t idx) const noexcept;

    [[nodiscard]] double child_score(const std::size_t idx) const noexcept;

    void create_children(const libchess::Position& pos,
                         const libchess::MoveList& move_list) noexcept;

   private:
    double score_;
    int visits_;
    libchess::Move move_;
    bool is_terminal_;
    UCTNode* parent_;
    unsigned visited_children_;
    std::vector<UCTNode> children_;
    std::vector<double> probabilities_;
};

}  // namespace megumax

#endif  // MEGUMAX_MCTS_UCT_NODE_H
