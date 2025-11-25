#pragma once
#include <cstddef>
#include <cstdlib>
#include <new>
#include <mutex>
#include <memory>

// This Pool Allocator was completly written by ChatGTP version 5.1 (Accessed: 24.11.2025)
// C++17 compatible Pool Allocator
// Fixed chunk size per T
// Uses a free list to recycle memory
//
// Designed as a drop-in std::allocator replacement
//

namespace ksets {
namespace memory {

struct NoMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};



template <typename T, std::size_t BlockSize = 4096, bool ThreadSafe = false>
class PoolAllocator
{
private:
    union FreeNode {
        FreeNode* next;
        alignas(T) unsigned char storage[sizeof(T)];
    };

    FreeNode* free_list_ = nullptr;

    // A block contains many nodes
    struct Block {
        Block* next;
        FreeNode nodes[BlockSize];
    };

    Block* blocks_ = nullptr;

    using Lock = typename std::conditional<ThreadSafe, std::mutex, struct NoMutex>::type;
    mutable Lock lock_;

    struct Guard {
        Lock& mtx;

        Guard(Lock& m) : mtx(m) {
            mtx.lock();   // For NoMutex: does nothing
        }
        ~Guard() {
            mtx.unlock(); // For NoMutex: does nothing
        }
    };

public:
    using value_type = T;

    PoolAllocator() noexcept = default;

    template <class U>
    PoolAllocator(const PoolAllocator<U, BlockSize, ThreadSafe>&) noexcept {}

    ~PoolAllocator() {
        // Free all allocated blocks
        Block* b = blocks_;
        while (b) {
            Block* next = b->next;
            std::free(b);
            b = next;
        }
    }

    T* allocate(std::size_t n) {
        if (n != 1)
            throw std::bad_alloc();  // fixed-size allocator

        Guard g(lock_);

        if (free_list_) {
            FreeNode* node = free_list_;
            free_list_ = free_list_->next;
            return reinterpret_cast<T*>(node);
        }

        // Allocate new block
        Block* new_block = static_cast<Block*>(std::malloc(sizeof(Block)));
        if (!new_block)
            throw std::bad_alloc();

        new_block->next = blocks_;
        blocks_ = new_block;

        // link all nodes except the first (which we will use now)
        for (std::size_t i = 1; i < BlockSize; ++i) {
            new_block->nodes[i].next = free_list_;
            free_list_ = &new_block->nodes[i];
        }

        return reinterpret_cast<T*>(&new_block->nodes[0]);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        if (n != 1) return; // fixed-size only

        Guard g(lock_);
        FreeNode* node = reinterpret_cast<FreeNode*>(p);
        node->next = free_list_;
        free_list_ = node;
    }

    template <class U>
    struct rebind {
        using other = PoolAllocator<U, BlockSize, ThreadSafe>;
    };
};


//
// Comparison operators (STL requires these to be equal across allocators
// of same type so containers can interact)
//
template <class T, std::size_t B, bool TS>
bool operator==(const PoolAllocator<T,B,TS>&, const PoolAllocator<T,B,TS>&) noexcept {
    return true;
}
template <class T, std::size_t B, bool TS>
bool operator!=(const PoolAllocator<T,B,TS>&, const PoolAllocator<T,B,TS>&) noexcept {
    return false;

}
} // namespace memory
} // namespace ksets
