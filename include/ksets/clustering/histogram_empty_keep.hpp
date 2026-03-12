#pragma once

#include <vector>
#include <ksets/clustering/histogram_empty_policy.hpp>
#include <ksets/types.hpp>
#include <tuple>

namespace ksets {
    class EmptyClusterKeep
            : public EmptyClusterPolicy<EmptyClusterKeep>{

        public: 
            using Base = EmptyClusterPolicy<EmptyClusterKeep>;
            using Base::Base;
            explicit EmptyClusterKeep(std::vector<cluster_t> &labels, size_t K) 
                : Base(labels, K) {}
        
            void begin_iteration_impl(size_t iter) {}

            void register_distances_impl(size_t row, auto const& dists){}

            void register_move_impl(size_t row, cluster_t old_cluster, cluster_t new_cluster, dist_base_t dist){}

            void execute_fix_impl(std::vector<cluster_t>& labels_cur){}
                              

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