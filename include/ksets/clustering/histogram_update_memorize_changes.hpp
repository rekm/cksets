#pragma once
#include <ksets/clustering/histogram_update_policy.hpp>
#include <ksets/types.hpp>

namespace ksets {

    template <typename HistogramCollection>
    class HistUpdaterMemorizeChanges
        : public HistogramUpdatePolicy<HistUpdaterMemorizeChanges<HistogramCollection>, HistogramCollection>
    {
    public:
        using Base = HistogramUpdatePolicy<HistUpdaterMemorizeChanges<HistogramCollection>,
                            HistogramCollection>;
        using Base::Base;
        using Base::iteration_;
        explicit HistUpdaterMemorizeChanges(HistogramCollection& hist,
                            const std::vector<std::vector<token_t>>& data)
            : Base(hist, data),  
            hists_(hist), data_(data)
        {}

        void before_iteration_impl(size_t it_idx) {
        }

        void register_move_impl(size_t row, size_t from, size_t to) {
            changes_.emplace_back(row, from, to);
        }

        void commit_impl(std::vector<int>& labels_cur) {
            // Apply all buffered changes
            //std::cout << std::format("Comitting for iteration {0:d}\n", Base::iteration_);
            for (auto const& change : changes_){
                size_t row_idx, old_cluster, new_cluster;
                std::tie(row_idx, old_cluster, new_cluster) = change;
                //labels_cur[row_idx] = new_cluster;
                
                #ifdef DEBUG_DESPERATION
                    std::cout << std::format("D_change[{0:d}] = {1:d} -> {2:d}\n", row_idx, old_cluster, new_cluster);
                #endif
                for (auto const& token : data_[row_idx]) {
                    if (Base::iteration_ != 0)
                        hists_.decrement(old_cluster, token);
                    hists_.increment(new_cluster, token);
                }
            }
            prev_changes_.swap(changes_);
            changes_.clear();
        }

        void rollback_impl(std::vector<int>& labels_cur) {
            // Undo last commit
            for (auto it = prev_changes_.rbegin(); it != prev_changes_.rend(); ++it){
                size_t row_idx, old_cluster, new_cluster;
                std::tie(row_idx, old_cluster, new_cluster) = *it;
                labels_cur[row_idx] = old_cluster;
                for (auto const& token : data_[row_idx]) {
                    hists_.decrement(new_cluster, token);
                    if ( Base::iteration_ != 0)
                        hists_.increment(old_cluster, token);
                }
            } 
        }

    private:
        HistogramCollection& hists_;
        const std::vector<std::vector<token_t>>& data_;
        // Tracking changes for histogram/cluster reassignment
        std::vector<std::tuple<size_t, size_t, size_t>> changes_;
        std::vector<std::tuple<size_t, size_t, size_t>> prev_changes_;
    };

} // namespace ksets


// Logic for solving empty cluster problem


//for (auto v : most_distant)
        //    std::cout << std::dec << v.first << " , " << v.second << '\n';

        /*
        for (auto const& empty_cluster : clusters_empty) {
            // choose donor cluster: global max of most_distant
            std::cout << "Cluster: " << empty_cluster.first << " is empty!\n";
            std::cout << "Dist: " << empty_cluster.second << " \n"; 
            auto it = std::max_element(
                most_distant.begin(), most_distant.end(),
                [](auto const &a, auto const &b) { return a.first < b.first; }
            );
            size_t donor_row = it->second;
            size_t donor_cluster = labels_cur[donor_row];
            //int donor_cluster = std::distance(most_distant.begin(), it);
            //size_t donor_row  = it->second;

            // move this row
            //labels_cur[donor_row] = empty_cluster.first;

            // update histogram changes
            histogram_changes.emplace_back(donor_row, donor_cluster, empty_cluster.first);

            // update sizes
            cluster_sizes[donor_cluster]--;
            cluster_sizes[empty_cluster.first]++;

            sumd[donor_cluster] -= empty_cluster.second;

            // distance contributed by this row should no longer be considered
            it->first = 0.0;      // cannot be used again
            it->second = 0;       // dummy index
        }
        */