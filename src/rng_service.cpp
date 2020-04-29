#include "rng_service.h"

namespace megumax {

std::unique_ptr<RNGService> RNGService::instance_ = nullptr;

RNGService::RNGService() : rng_(std::random_device{}()) {
}

RNGService* RNGService::singleton() {
    if (instance_ == nullptr) {
        instance_ = std::make_unique<RNGService>();
    }
    return instance_.get();
}

std::uint32_t RNGService::rand_uint32(std::uint32_t low, std::uint32_t high) {
    std::uniform_int_distribution<std::uint32_t> dist(low, high);
    return dist(rng_);
}

}  // namespace megumax
