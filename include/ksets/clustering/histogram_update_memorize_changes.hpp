#pragma once
#include <ksets/clustering/histogram_update_policy.hpp>
#include <ksets/types.hpp>
#include <ksets/telemetry/base.hpp>
#include <vector>

namespace ksets {

    


    template <typename HistogramCollection, telemetry::TelemetrySink Sink>
    class HistUpdaterMemorizeChanges
        : public HistogramUpdatePolicy<HistUpdaterMemorizeChanges<HistogramCollection, Sink>, HistogramCollection, Sink>
    {
    public:
        using Base = HistogramUpdatePolicy<HistUpdaterMemorizeChanges<HistogramCollection, Sink>,
                            HistogramCollection, Sink>;
        using Base::Base;
        using Base::iteration_;
        explicit HistUpdaterMemorizeChanges(HistogramCollection& hist,
                            const std::vector<std::vector<token_t>>& data,
                            const Sink& sink, 
                            bool histograms_provided = false
                            )
            : Base(hist, data, sink, histograms_provided),  
            hists_(hist), data_(data), histograms_provided_(histograms_provided)
        {}

        void before_iteration_impl(size_t it_idx) {
        }

        void register_move_impl(size_t row, size_t from, size_t to) {
            changes_.emplace_back(row, from, to);
        }

        void commit_impl(std::vector<label_t>& labels_cur) {
            // Apply all buffered changes
            //std::cout << std::format("Comitting for iteration {0:d}\n", Base::iteration_);
            if (histograms_provided_ && (Base::iteration_ == 0)) {
                hists_.clear();
                for (size_t i = 0; i < data_.size(); i++) {
                    for (auto const& token : data_[i]) {
                        hists_.increment(labels_cur[i], token);
                    }
                }
            } else {
                for (auto & change : changes_){
                    auto & [row_idx, old_cluster, new_cluster, old_token_removed, new_token_created] = change;
                    for (auto const& token : data_[row_idx]) {
                        //if (histograms_provided_ || (Base::iteration_ != 0))
                        old_token_removed = hists_.decrement(old_cluster, token);
                        new_token_created = hists_.increment(new_cluster, token);
                    }
                }
                telemetry::emit(Base::sink_, [&]{ 
                    return HistogramUpdateInfo{Base::iteration_, changes_.size()}; 
                }); 
                prev_changes_.swap(changes_);
            }
            
            changes_.clear();
        }

        void rollback_impl(std::vector<label_t>& labels_cur) {
            // Undo last commit
            for (auto it = prev_changes_.rbegin(); it != prev_changes_.rend(); ++it){
                auto & [row_idx, old_cluster, new_cluster, old_token_removed, new_token_created] = *it;
                labels_cur[row_idx] = old_cluster;
                for (auto const& token : data_[row_idx]) {
                    hists_.decrement(new_cluster, token);
                    if ( Base::iteration_ != 0)
                        hists_.increment(old_cluster, token);
                }
            } 
        }

        std::vector<Change>& get_changes() {
            return changes_;
        }

    private:
        bool histograms_provided_ = false;
        HistogramCollection& hists_;
        const std::vector<std::vector<token_t>>& data_;
        // Tracking changes for histogram/cluster reassignment
        std::vector<Change> changes_;
        std::vector<Change> prev_changes_;
    };

} // namespace ksets


