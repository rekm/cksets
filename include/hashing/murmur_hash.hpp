
#pragma once

#include <string>
#include <cstdint>
#include "hashing/MurmurHash3.h"

namespace ksets {

    struct MurmurHash
    {
        uint32_t seed;
        
        explicit MurmurHash(uint32_t seed_ = 42) : seed(seed_) {}

        std::size_t operator()(std::string const &key) const noexcept {
            uint32_t out; // MurmurHash3_x86_32 writes 32 bits = 4 bytes
            MurmurHash3_x86_32(key.data(), key.size(), seed, &out);
            return static_cast<size_t>(out);
        }
    };

}