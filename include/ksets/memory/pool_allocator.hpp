#pragma once
#include <cstddef>
#include <cstdlib>
#include <new>
#include <mutex>
#include <memory>
#include <type_traits>

// Optimized C++17 compatible Pool Allocator
// Fixed chunk size per T with fallback for n != 1
// Uses shared state management to safely handle container copies/rebinds without segfaults

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

    struct Block {
        Block* next;
        FreeNode nodes[BlockSize];
    };

    using Lock = typename std::conditional<ThreadSafe, std::mutex, struct NoMutex>::type;

    // Encapsulate the core allocation state in a shared structure to handle copying safely
    struct PoolState {
        FreeNode* free_list = nullptr;
        Block* blocks = nullptr;
        mutable Lock mtx;

        ~PoolState() {
            Block* b = blocks;
            while (b) {
                Block* next = b->next;
                std::free(b);
                b = next;
            }
        }
    };

    std::shared_ptr<PoolState> state_;

public:
    using value_type = T;

    // Default Constructor
    PoolAllocator() : state_(std::make_shared<PoolState>()) {}

    // Copy Constructor (Same Type): Shares the underlying memory pool state
    PoolAllocator(const PoolAllocator& other) noexcept : state_(other.state_) {}

    // Move Constructor
    PoolAllocator(PoolAllocator&& other) noexcept : state_(std::move(other.state_)) {}

    template <typename U>
    struct rebind {
        using other = PoolAllocator<U, BlockSize, ThreadSafe>;
    };

    // Copy Assignment
    PoolAllocator& operator=(const PoolAllocator& other) noexcept {
        if (this != &other) {
            state_ = other.state_;
        }
        return *this;
    }

    // Move Assignment
    PoolAllocator& operator=(PoolAllocator&& other) noexcept {
        if (this != &other) {
            state_ = std::move(other.state_);
        }
        return *this;
    }

    // Converting Template Constructor for Rebinding (e.g., std::list node allocations)
    template <class U>
    PoolAllocator(const PoolAllocator<U, BlockSize, ThreadSafe>&) : state_(std::make_shared<PoolState>()) {}

    ~PoolAllocator() = default;

    T* allocate(std::size_t n) {
        // Fallback to standard heap allocation if multiple contiguous elements are requested
        if (n != 1) {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }

        std::lock_guard<Lock> g(state_->mtx);

        // Serve from existing free list if available
        if (state_->free_list) {
            FreeNode* node = state_->free_list;
            state_->free_list = state_->free_list->next;
            return reinterpret_cast<T*>(node);
        }

        // Allocate a new full block of nodes
        Block* new_block = static_cast<Block*>(std::malloc(sizeof(Block)));
        if (!new_block) {
            throw std::bad_alloc();
        }

        // Initialize block tracking link
        new_block->next = state_->blocks;
        state_->blocks = new_block;

        // Build free list out of the newly allocated block elements
        // Node 0 will be handed directly to the caller, rest go to free list
        for (std::size_t i = 1; i < BlockSize; ++i) {
            new_block->nodes[i].next = state_->free_list;
            state_->free_list = &new_block->nodes[i];
        }

        return reinterpret_cast<T*>(&new_block->nodes[0]);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        if (!p) return;

        // Handle custom sized cleanups via global operator delete
        if (n != 1) {
            ::operator delete(p);
            return;
        }

        std::lock_guard<Lock> g(state_->mtx);

        // Return chunk back to the recycling list
        FreeNode* node = reinterpret_cast<FreeNode*>(p);
        node->next = state_->free_list;
        state_->free_list = node;
    }

    // Equality operators for standard compliance
    bool operator==(const PoolAllocator& other) const noexcept {
        return state_ == other.state_;
    }

    bool operator!=(const PoolAllocator& other) const noexcept {
        return state_ != other.state_;
    }

    template <typename U>
    bool operator==(const PoolAllocator<U, BlockSize, ThreadSafe>&) const noexcept {
        return false;
    }

    template <typename U>
    bool operator!=(const PoolAllocator<U, BlockSize, ThreadSafe>&) const noexcept {
        return true;
    }
};

} // namespace memory
} // namespace ksets
