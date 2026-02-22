#pragma once 
#include <ksets/types.hpp> // for token_t, move_t


namespace ksets::distance {

    template<class Derived, class HistogramCollection>
    class DistancePolicy {
    protected:
        HistogramCollection& H_;

    public:
        explicit DistancePolicy(HistogramCollection& H)
            : H_(H) {}

        void begin_iteration(size_t iter){
            static_cast<Derived*>(this)->begin_iteration_impl(iter);
        }

        void begin_row(size_t K) {
            static_cast<Derived*>(this)->begin_row_impl(K);
        }

        template <class Range>
        void accumulate_token(const token_t& token, const Range& range) {
            static_cast<Derived*>(this)->accumulate_token_impl(token, range);
        } 

        const std::vector<double>& finalize() {
            return static_cast<Derived*>(this)->finalize_impl();
        }
    };

} // ksets::distance