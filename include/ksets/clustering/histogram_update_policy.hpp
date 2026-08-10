#pragma once
#include <ksets/types.hpp>
#include <ksets/telemetry/base.hpp>
#include <iostream>
#include <vector>

namespace ksets {

    struct HistogramUpdateInfo {
        static constexpr telemetry::TelemetryLevel level =
            telemetry::TelemetryLevel::basic;
        size_t iteration;
        size_t changes;
    
    };

    template <typename Derived, class HistogramCollection, telemetry::TelemetrySink Sink>
    class HistogramUpdatePolicy {
        protected:
            HistogramCollection& H_;
            const std::vector<std::vector<token_t>>& data_;
            iteration_count_t iteration_ = 0;
            bool histograms_provided_ = false; 
            const Sink& sink_;   
        public:
            HistogramUpdatePolicy(
                        HistogramCollection& h,
                        const std::vector<std::vector<token_t>>& d,
                        const Sink& sink,
                        bool histograms_provided = false
                        )
                : H_(h), data_(d), histograms_provided_(histograms_provided), sink_(sink){}

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

            void commit(std::vector<label_t>& labels_cur) {
                derived().commit_impl(labels_cur);
            }

            void rollback(std::vector<label_t>& labels_cur) {
                derived().rollback_impl(labels_cur);
            }
            
        private:
            Derived& derived() { return static_cast<Derived&>(*this); }
            const Derived& derived() const { return static_cast<const Derived&>(*this); }
    };


}
