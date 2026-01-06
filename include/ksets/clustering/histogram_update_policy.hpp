#pragma once
#include <ksets/types.hpp>
#include <iostream>

namespace ksets {

    template <typename Derived, class HistogramCollection>
    class HistogramUpdatePolicy {
        protected:
            HistogramCollection& H_;
            const std::vector<std::vector<token_t>>& data_;
            int iteration_ = 0;   
        public:
            HistogramUpdatePolicy(HistogramCollection& h,
                                const std::vector<std::vector<token_t>>& d)
                : H_(h), data_(d) {}

            void before_iteration(size_t it_idx) {
                #ifdef DEBUG_BASE
                    if (it_idx % 1 == 0){
                        std::cout << "\nin progress... " << it_idx << "\n";
                    }
                #endif
                #ifdef DEBUG_DESPERATION
                    for (size_t idx=0; idx < H_.size(); ++idx)
                        H_[idx].debug_print();
                #endif
                iteration_ = it_idx;
                derived().before_iteration_impl(it_idx);
            }

            void register_move(size_t row, size_t from, size_t to) {
                derived().register_move_impl(row, from, to);
            }

            void commit(std::vector<int>& labels_cur) {
                derived().commit_impl(labels_cur);
            }

            void rollback(std::vector<int>& labels_cur) {
                derived().rollback_impl(labels_cur);
            }
            
        private:
            Derived& derived() { return static_cast<Derived&>(*this); }
            const Derived& derived() const { return static_cast<const Derived&>(*this); }
    };


}
