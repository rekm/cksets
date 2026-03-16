#pragma once
#include <memory>


// Mostly ChatGTP Version 5, accessed 16. March 2026 and prompted to complete the allocator s
// signature based on an error of missing methods in structure. 
namespace ksets::memory {
    template<typename T, std::size_t Align>
    struct AlignedAllocator {
        using value_type = T;

        AlignedAllocator() noexcept = default;

        template <class U>
        constexpr AlignedAllocator(const AlignedAllocator<U, Align>&) noexcept {}

        [[nodiscard]] T* allocate(std::size_t n) {
            std::size_t size = n * sizeof(T);

            // aligned_alloc requires size to be multiple of alignment
            std::size_t aligned_size =
                ((size + Align - 1) / Align) * Align;

            void* ptr = std::aligned_alloc(Align, aligned_size);
            if (!ptr) throw std::bad_alloc();
            return static_cast<T*>(ptr);
        }

        void deallocate(T* p, std::size_t) noexcept {
            std::free(p);
        }

        template<class U>
        struct rebind {
            using other = AlignedAllocator<U, Align>;
        };
    };

    template<class T1, size_t A1, class T2, size_t A2>
    inline bool operator==(const AlignedAllocator<T1, A1>&,
                        const AlignedAllocator<T2, A2>&) noexcept {
        return A1 == A2;
    }

    template<class T1, size_t A1, class T2, size_t A2>
    inline bool operator!=(const AlignedAllocator<T1, A1>& a,
                        const AlignedAllocator<T2, A2>& b) noexcept {
        return !(a == b);
    }


} // namespace ksets::memory