#pragma once

#include <cstddef>
#include <new>

#ifdef __linux__
#include <sys/mman.h>
#endif

// Stateless std::allocator that backs allocations with huge pages when available,
// falling back to a plain anonymous mapping advised with MADV_HUGEPAGE.
namespace algos::rfd {

template <typename T>
struct HugePageAllocator {
    using value_type = T;

    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        std::size_t const bytes = n * sizeof(T);
#ifdef __linux__
        void* p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        if (p == MAP_FAILED) {
            p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (p != MAP_FAILED) madvise(p, bytes, MADV_HUGEPAGE);
        }
        if (p == MAP_FAILED) throw std::bad_alloc{};
        return static_cast<T*>(p);
#else
        return static_cast<T*>(::operator new(bytes));
#endif
    }

    void deallocate(T* p, std::size_t n) {
#ifdef __linux__
        if (p != nullptr) munmap(p, n * sizeof(T));
#else
        ::operator delete(p);
#endif
    }

    template <typename U>
    struct rebind {
        using other = HugePageAllocator<U>;
    };

    bool operator==(HugePageAllocator const&) const noexcept {
        return true;
    }
};

}  // namespace algos::rfd