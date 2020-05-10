#ifndef MEGUMAX_SEARCH_GLOBALS_H
#define MEGUMAX_SEARCH_GLOBALS_H

#include <condition_variable>
#include <mutex>

#include "libchess/Position.h"
#include "libchess/UCIService.h"

#include "misc.h"

namespace megumax {

class SearchGlobals {
   public:
    SearchGlobals(std::uint64_t nodes,
                  std::optional<std::chrono::milliseconds> start_time,
                  std::optional<libchess::UCIGoParameters> go_parameters) noexcept;

    static SearchGlobals new_search_globals(
        const std::optional<std::chrono::milliseconds>& start_time = {},
        const std::optional<libchess::UCIGoParameters>& go_parameters = {}) noexcept;

    [[nodiscard]] bool searching() const noexcept;
    [[nodiscard]] bool debug() const noexcept;
    [[nodiscard]] std::uint64_t nodes() const noexcept;
    [[nodiscard]] const std::optional<libchess::UCIGoParameters>& go_parameters() const noexcept;

    void reset_nodes() noexcept;
    void start_time(std::chrono::milliseconds start_time) noexcept;
    void go_parameters(const libchess::UCIGoParameters& go_parameters) noexcept;
    void searching(bool searching) noexcept;
    void debug(bool debug) noexcept;
    void stop_flag(bool stop_flag) noexcept;
    void side_to_move(libchess::Color color) noexcept;

    void increment_nodes() noexcept;
    [[nodiscard]] bool stop() noexcept;

   public:
    std::mutex debug_mutex;
    std::condition_variable debug_cv;

   private:
    libchess::Color side_to_move_;
    std::atomic<bool> searching_;
    std::atomic<bool> stop_flag_;
    std::atomic<std::uint64_t> nodes_;
    std::optional<std::chrono::milliseconds> start_time_;
    std::optional<libchess::UCIGoParameters> go_parameters_;

    bool debug_;
};

}  // namespace megumax

#endif  // MEGUMAX_SEARCH_GLOBALS_H
