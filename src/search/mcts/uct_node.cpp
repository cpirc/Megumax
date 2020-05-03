#include "uct_node.h"

namespace megumax {

UCTNode::UCTNode(libchess::Move move, UCTNode* parent)
    : score_(0.0),
      visits_(0),
      move_(move),
      is_terminal_(false),
      parent_(parent),
      visited_children_(0),
      children_(),
      probabilities_() {
}

double UCTNode::p(const libchess::Position& pos) const noexcept {
    assert(pos.is_legal_move(move_));
    double score = 0.0;

    // MVV-LVA
    if (move_.type() == libchess::Move::Type::ENPASSANT) {
        score = (libchess::constants::PAWN.value() + 1) * 10 - libchess::constants::PAWN.value();
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

double UCTNode::score() const {
    return score_;
}

void UCTNode::add_score(const double n) {
    score_ += n;
}

int UCTNode::visits() const {
    return visits_;
}

void UCTNode::increment_visits() {
    ++visits_;
}

const libchess::Move& UCTNode::move() const {
    return move_;
}

bool UCTNode::is_terminal() const {
    return is_terminal_;
}

void UCTNode::is_terminal(bool is_terminal) {
    is_terminal_ = is_terminal;
}

UCTNode* UCTNode::parent() const {
    return parent_;
}

unsigned UCTNode::visited_children() const {
    return visited_children_;
}

void UCTNode::increment_visited_children() {
    ++visited_children_;
}

std::vector<UCTNode>& UCTNode::children() {
    return children_;
}

const std::vector<UCTNode>& UCTNode::children() const noexcept {
    return children_;
}

int UCTNode::depth() const {
    const UCTNode* iter = this;
    int ply = 0;
    while (iter->parent_ != nullptr) {
        iter = iter->parent();
        ++ply;
    }
    return ply;
}

double UCTNode::child_probability(std::size_t idx) const noexcept {
    return probabilities_.at(idx);
}

double UCTNode::child_score(std::size_t idx) const noexcept {
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

void UCTNode::create_children(const libchess::Position& pos,
                              const libchess::MoveList& move_list) noexcept {
    children_.reserve(move_list.size());
    probabilities_.reserve(move_list.size());

    double sum = 0.0;
    for (const libchess::Move& move : move_list.values()) {
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

}  // namespace megumax
