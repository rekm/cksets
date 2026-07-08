#pragma once

#include <cstdint>
#include <ksets/types.hpp>
#include <ksets/hashing/hash_policy.hpp>


namespace ksets {
namespace hashing {

    struct IdentityHash
    {
        uint32_t seed;
        
        explicit IdentityHash(uint32_t seed_ = 42) : seed(seed_) {}

        size_t operator()(uint64_t x) const noexcept{
            return x;
        }

    };

}
}