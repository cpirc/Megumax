#ifndef MEGUMAX_MISC_H
#define MEGUMAX_MISC_H

#include <chrono>

namespace megumax {

static inline std::chrono::milliseconds curr_time() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
}

}  // namespace megumax

#endif  // MEGUMAX_MISC_H
