#include "search_globals.h"

namespace megumax {

SearchGlobals SearchGlobals::new_search_globals(
    const std::optional<std::chrono::milliseconds>& start_time,
    const std::optional<libchess::UCIGoParameters>& go_parameters) noexcept {
    return SearchGlobals{0, start_time, go_parameters};
}

SearchGlobals::SearchGlobals(std::uint64_t nodes,
                             std::optional<std::chrono::milliseconds> start_time,
                             std::optional<libchess::UCIGoParameters> go_parameters) noexcept
    : side_to_move_(libchess::constants::WHITE),
      stop_flag_(false),
      nodes_(nodes),
      start_time_(start_time),
      go_parameters_(std::move(go_parameters)) {
}

std::uint64_t SearchGlobals::nodes() const noexcept {
    return nodes_;
}

const std::optional<libchess::UCIGoParameters>& SearchGlobals::go_parameters() const noexcept {
    return go_parameters_;
}

void SearchGlobals::reset_nodes() noexcept {
    nodes_ = 0;
}

void SearchGlobals::start_time(std::chrono::milliseconds start_time) noexcept {
    start_time_ = start_time;
}

void SearchGlobals::go_parameters(const libchess::UCIGoParameters& go_parameters) noexcept {
    go_parameters_ = go_parameters;
}

void SearchGlobals::stop_flag(bool stop_flag) noexcept {
    stop_flag_ = stop_flag;
}

void SearchGlobals::side_to_move(libchess::Color color) noexcept {
    side_to_move_ = color;
}

void SearchGlobals::increment_nodes() noexcept {
    ++nodes_;
}

bool SearchGlobals::stop() noexcept {
    if (stop_flag_) {
        return true;
    }
    if (!go_parameters_) {
        return false;
    }
    if (!(nodes_ & 127U) && start_time_) {
        auto time_diff = curr_time().count() - start_time_->count();

        auto time = [this]() {
            if (side_to_move_ == libchess::constants::WHITE) {
                return go_parameters_->wtime();
            } else {
                return go_parameters_->btime();
            }
        }();
        auto inc = [this]() {
            if (side_to_move_ == libchess::constants::WHITE) {
                return go_parameters_->winc();
            } else {
                return go_parameters_->binc();
            }
        }();
        auto movestogo = go_parameters_->movestogo();
        if (!movestogo) {
            movestogo = {30};
        }

        if (go_parameters_->infinite()) {
            return false;
        } else if (time && inc) {
            long end_time = (*time + (*movestogo - 1) * *inc) / *movestogo;
            if (*movestogo == 1) {
                end_time -= 50;
            }
            if (curr_time().count() - start_time_->count() >= end_time) {
                stop_flag_ = true;
            }
        } else if (go_parameters_->movetime() && time_diff >= go_parameters_->movetime()) {
            stop_flag_ = true;
        }
    }

    return stop_flag_;
}

}  // namespace megumax
