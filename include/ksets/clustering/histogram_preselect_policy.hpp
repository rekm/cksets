#pragma once
#include <ksets/histogram/collection_iterator.hpp>
#include <ksets/types.hpp>
#include <iostream>


namespace ksets {

    template<class T>
    concept SupportsQuickLabelEstimate =
    requires(T t, std::vector<token_t> const& row)
    {
        { t.estimate_best_cluster(row) } -> std::convertible_to<label_t>;
    };

    template <typename Derived, class HistogramCollection, class Distance>
    class HistogramPreselectPolicy {
        protected:
            HistogramCollection& H_;
            const std::vector<std::vector<token_t>>& data_;
            int iteration_ = 0;   
        public:
            HistogramPreselectPolicy(HistogramCollection& h,
                                const std::vector<std::vector<token_t>>& d, Distance &dist)
                : H_(h), data_(d), dist_(dist) {}

            void before_iteration(size_t it_idx, std::vector<label_t>& labels_cur) {
                iteration_ = it_idx;
                derived().before_iteration_impl(it_idx, labels_cur);
            }

            auto get_candidate_histograms(const std::vector<token_t>& data_row, size_t row_idx) {
                return derived().get_candidate_histograms_impl(data_row, row_idx);
            }

            void update(std::vector<label_t>& labels_cur) {
                derived().update_impl(labels_cur);
            }
        protected:
            Distance& dist_; 
        private:
            
            Derived& derived() { return static_cast<Derived&>(*this); }
            const Derived& derived() const { return static_cast<const Derived&>(*this); }
    };


}
