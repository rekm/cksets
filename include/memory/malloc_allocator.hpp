#pragma once
#include <memory>

namespace ksets {
namespace memory {

struct MallocAllocator {
    static void* allocate(std::size_t n) {
        return std::malloc(n);
    }

    static void deallocate(void* p) {
        std::free(p);
    }
};

} // namespace memory 
} // namespace ksets