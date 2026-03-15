#pragma once
#include <ksets/types.hpp>

namespace ksets::histogram {
    
    template <class HistogramCollection, typename IndexContainer>
    class CollectionIterator {
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = size_t;                    // what operator* returns
        using pointer           = const size_t*;
        using reference         = const size_t&;
        using iterator_category = std::forward_iterator_tag;
        using index_list_t = IndexContainer;
        // ChatGTP deconvoluted the access to the underlying histogram type
        //using histogram_ref = decltype(std::declval<HistogramCollection*>()[0]);
        //using histogram_type = std::remove_reference_t<histogram_ref>;
        // but not used in the end 
        //using value_type = std::pair<size_t, histogram_type&>;

        CollectionIterator(HistogramCollection* H,
                                const index_list_t* indices,
                                size_t pos)
            : H_(H), indices_(indices), pos_(pos) {}

        reference operator*() const noexcept { 
            return (*indices_)[pos_]; 
        }

        pointer operator->() const noexcept { 
            return &(*indices_)[pos_]; 
        }

        CollectionIterator& operator++() noexcept { 
            ++pos_; 
            return *this; 
        }

        CollectionIterator operator++(int) noexcept {
            CollectionIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const CollectionIterator& other) const noexcept {
            return pos_ == other.pos_;
        }

        bool operator!=(const CollectionIterator& other) const noexcept {
            return !(*this == other);
        }

    private:
        HistogramCollection* H_;
        const index_list_t* indices_;
        size_t pos_;
    };

    template <class HistogramCollection, typename IndexContainer>
    struct CollectionRange {
        using index_list_t = IndexContainer;
        using iterator = CollectionIterator<HistogramCollection, IndexContainer>;
        
        HistogramCollection* H_;
        const index_list_t indices;

        iterator begin() const { return iterator{H_, &indices, 0}; }
        iterator end()   const { return iterator{H_, &indices, indices.size()}; }
        size_t size() const { return indices.size(); }
    };

}