#pragma once
#include <ksets/clustering/histogram_update_policy.hpp>
#include <ksets/types.hpp>
#include <vector>

namespace ksets {

    template <typename HistogramCollection, telemetry::TelemetrySink Sink>
    class HistUpdaterKsetsAuthors
        : public HistogramUpdatePolicy<HistUpdaterKsetsAuthors<HistogramCollection, Sink>, HistogramCollection, Sink>
    {
    public:
        using Base = HistogramUpdatePolicy<HistUpdaterKsetsAuthors<HistogramCollection, Sink>,
                            HistogramCollection, Sink>;
        using Base::Base;
        using Base::iteration_;
        explicit HistUpdaterKsetsAuthors(
                        HistogramCollection& hist,
                        const std::vector<std::vector<token_t>>& data, 
                        const Sink & sink,
                        bool histograms_provided = false
                        )
            : Base(hist, data, sink, histograms_provided),  
            hists_(hist), data_(data), prev_hists_(hist.clone_empty())
        {}

        void before_iteration_impl(size_t it_idx) {}

        void register_move_impl(size_t row, size_t from, size_t to) {}

        void commit_impl(std::vector<label_t>& labels_cur) {
            // Apply all buffered changes
            prev_hists_.clear();
            swap(prev_hists_, hists_);
            hists_.clear();
            for (size_t i = 0; i < data_.size(); i++) {
                for (auto const& token : data_[i]) {
                    hists_.increment(labels_cur[i], token);
                }
            }
            telemetry::emit(Base::sink_, [&]{
                    size_t t_size = 0;
                    for (size_t i=0; i<hists_.size(); i++) 
                        t_size += hists_[i].size();
                    return HistogramUpdateInfo{Base::iteration_, t_size}; 
            }); 
        }

        void rollback_impl(std::vector<label_t>& labels_cur) {
            // Undo last commit
            hists_.clear();
            swap(hists_, prev_hists_);
        }

    private:
        HistogramCollection& hists_;
        const std::vector<std::vector<token_t>>& data_;
        HistogramCollection prev_hists_;
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