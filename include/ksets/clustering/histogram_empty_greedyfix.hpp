#pragma once

#include <vector>
#include <ksets/clustering/histogram_empty_policy.hpp>
#include <ksets/types.hpp>
#include <tuple>

namespace ksets {
    class EmptyClusterGreedyFix
            : public EmptyClusterPolicy<EmptyClusterGreedyFix>{

        public: 
            using Base = EmptyClusterPolicy<EmptyClusterGreedyFix>;
            using Base::Base;
            explicit EmptyClusterGreedyFix(std::vector<cluster_t> &labels, size_t K) 
                : Base(labels, K) {
                    cluster_sizes_ =  std::vector<size_t>(K_, 0);
                    most_distant_ = std::vector<std::pair<double, size_t>>(K_, {0.0, 0}); 
                }
        
            void begin_iteration_impl(size_t iter) {
                clusters_empty_.clear();
                std::fill(cluster_sizes_.begin(), cluster_sizes_.end(), 0);
                std::fill(most_distant_.begin(), most_distant_.end(), std::make_pair(0.0, 0));
            }

            void register_distances_impl(size_t row, auto const& dists){
                // This is wrong and makes no sense
                // Todo: Rework this logic
                for (size_t c = 0; c < K_; ++c) {
                    if (dists[c] > most_distant_[c].first) {
                        most_distant_[c] = { dists[c], row };
                    }
                }
            }

            void register_move_impl(size_t row, cluster_t old_cluster, cluster_t new_cluster, dist_base_t dist){
                cluster_sizes_[old_cluster]--;  // decrement old cluster
                cluster_sizes_[new_cluster]++;
                if (cluster_sizes_[old_cluster] == 0)
                    // this also makes no sense 
                    // i need the row and new distance to keep my knowledge about the state intact
                    clusters_empty_.emplace_back(old_cluster, dist);
            }

            void execute_fix_impl(std::vector<cluster_t>& labels_cur){

            }

        private:
            std::vector<size_t> cluster_sizes_;
            // Storing most distant data to clusters
            std::vector<std::pair<size_t, double>> clusters_empty_;
            std::vector<std::pair<double, size_t>> most_distant_;
            
            

    };

}

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