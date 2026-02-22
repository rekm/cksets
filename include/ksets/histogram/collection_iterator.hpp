#pragma once
#include <ksets/types.hpp>

namespace ksets::histogram {
    
    template <class HistogramCollection>
    class CollectionIterator {
    public:
        using index_list_t = std::vector<size_t>;
        // ChatGTP deconvoluted the access to the underlying histogram type
        using histogram_ref = decltype(std::declval<HistogramCollection*>()[0]);
        using histogram_type = std::remove_reference_t<histogram_ref>;
        // but not used in the end 
        using value_type = std::pair<size_t, histogram_type&>;

        CollectionIterator(HistogramCollection* H,
                                const index_list_t* indices,
                                size_t pos)
            : H_(H), indices_(indices), pos_(pos) {}

        size_t operator*() const { 
            size_t idx = (*indices_)[pos_];
            return idx; 
        }

        CollectionIterator& operator++() {
            ++pos_;
            return *this;
        }

        bool operator!=(const CollectionIterator& other) const {
            return pos_ != other.pos_;
        }

    private:
        HistogramCollection* H_;
        const index_list_t* indices_;
        size_t pos_;
    };

    template <class HistogramCollection>
    struct CollectionRange {
        using index_list_t = std::vector<size_t>;
        using iterator = CollectionIterator<HistogramCollection>;
        
        HistogramCollection* H_;
        index_list_t indices;

        iterator begin() const { return iterator{H_, &indices, 0}; }
        iterator end()   const { return iterator{H_, &indices, indices.size()}; }
        size_t size() const { return indices.size(); }
    };

}