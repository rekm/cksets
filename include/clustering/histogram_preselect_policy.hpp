#pragma once
#include <histogram/collection_iterator.hpp>
#include <iostream>

namespace ksets {

    template <typename Derived, class HistogramCollection>
    class HistogramPreselectPolicy {
        protected:
            HistogramCollection& H_;
            const std::vector<std::vector<std::string>>& data_;
            int iteration_ = 0;   
        public:
            HistogramPreselectPolicy(HistogramCollection& h,
                                const std::vector<std::vector<std::string>>& d)
                : H_(h), data_(d) {}

            void before_iteration(size_t it_idx) {
                iteration_ = it_idx;
                derived().before_iteration_impl(it_idx);
            }

            auto get_candidate_histograms(const std::vector<std::string>& data_row) {
                return derived().get_candidate_histograms_impl(data_row);
            }

            void update(std::vector<int>& labels_cur) {
                derived().update_impl(labels_cur);
            }
            
        private:
            Derived& derived() { return static_cast<Derived&>(*this); }
            const Derived& derived() const { return static_cast<const Derived&>(*this); }
    };


}
