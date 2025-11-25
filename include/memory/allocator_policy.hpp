#pragma once 


namespace mylib::memory {

template <typename Derived>
struct AllocatorPolicy {
    using allocator_type = typename Derived::allocator_type;

    void* allocate(size_t n) {
        return static_cast<Derived*>(this)->allocate_impl(n);
    }

    void deallocate(void* p, size_t n) {
        static_cast<Derived*>(this)->deallocate_impl(p, n);
    }
};

} // namespace mylib::memory
