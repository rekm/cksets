
#pragma once

#include <string>
#include <cstdint>
#include <ksets/types.hpp>
#include <ksets/hashing/hash_policy.hpp>
#include "ksets/hashing/MurmurHash3.h"

namespace ksets {
namespace hashing {

    struct MurmurHash
    {
        uint32_t seed;
        
        explicit MurmurHash(uint32_t seed_ = 42) : seed(seed_) {}

        std::size_t operator()(std::string const &key) const noexcept {
            uint32_t out; // MurmurHash3_x86_32 writes 32 bits = 4 bytes
            MurmurHash3_x86_32(key.data(), key.size(), seed, &out);
            return static_cast<size_t>(out);
        }

        std::size_t operator()(token_t const &key) const noexcept {
            uint32_t out; // MurmurHash3_x86_32 writes 32 bits = 4 bytes
            MurmurHash3_x86_32(&key, sizeof(key), seed, &out);
            return static_cast<size_t>(out);
        }

    };

}
}