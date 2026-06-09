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

        double get(const std::vector<token_t>& token_row, cluster_t label) {
            return static_cast<Derived*>(this)->get_impl(token_row, label);
        }

        void begin_row() {
            static_cast<Derived*>(this)->begin_row_impl();
        }

        template <class Range>
        void accumulate_token(const token_t& token, const Range& range) {
            static_cast<Derived*>(this)->accumulate_token_impl(token, range);
        } 

        template <class Range>
        void accumulate_token(const std::vector<token_t>& token_row, const Range& range) {
            static_cast<Derived*>(this)->accumulate_token_impl(token_row, range);
        } 

        const std::vector<double>& finalize() {
            return static_cast<Derived*>(this)->finalize_impl();
        }
    };

} // ksets::distance