#ifndef MEGUMAX_RNG_SERVICE_H
#define MEGUMAX_RNG_SERVICE_H

#include <memory>
#include <random>

namespace megumax {

class RNGService {
   public:
    explicit RNGService();

    [[nodiscard]] static RNGService* singleton();

    [[nodiscard]] std::uint32_t rand_uint32(std::uint32_t low, std::uint32_t high);

   private:
    static std::unique_ptr<RNGService> instance_;

    std::mt19937 rng_;
};

}  // namespace megumax

#endif  // MEGUMAX_RNG_SERVICE_H
