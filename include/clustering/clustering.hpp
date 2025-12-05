
#pragma once
#include <histogram/base_histogram_collection.hpp>
#include <hashing/hash_policy.hpp>
#include <hashing/murmur_hash.hpp>
#include <string> 
#include <format>
#include <tuple>
#include <iostream> 
//using Histogram = std::unordered_map<std::string, int, ksets::MurmurHash>;

namespace ksets
{
    
struct ClusterResult {
    std::vector<int> labels;
    std::vector<std::vector<int>> histograms;
    std::vector<double> sumd;
};

template <class HistogramCollection>
std::vector<std::vector<int>> to_hist_output_format(
    const HistogramCollection& hist_vec,
    int max_hist_length
){
    std::vector<std::vector<int>> out;
    out.reserve(hist_vec.size());

    for (size_t i = 0; i < hist_vec.size(); ++i) {
        out.push_back(hist_vec[i].values(max_hist_length));
    }
    return out;
}

template <class HistogramCollection>
ClusterResult cluster_cpp(
        const std::vector<std::vector<std::string>> & data,
        int K,
        int max_hist_length,
        HistogramCollection& histograms,
        int max_iter = 50){
    
    if (max_iter == 0)
        max_iter = 50;

    std::vector<int> labels_cur(data.size());
    std::vector<int> labels_prev(data.size());
    // Build initial histograms
    if (histograms.empty()){
        for (size_t i = 0; i < data.size(); i++) {
            int cluster = i % K;
            labels_cur[i] = cluster;
            for (auto const& token : data[i]) {
                histograms.increment(cluster, token);
            }
        }
    }

    double sdth_old = 0.0;
    double sdth = std::numeric_limits<double>::max();
    std::vector<double> sumd_prev(K,0);
    std::vector<double> sumd(K,0);
    std::vector<int> hist_sums = histograms.totals();
    //for (size_t i = 0; i < hist_sums.size(); ++i) {
    //    std::cout << "Cluster " << i << " total: " << hist_sums[i] << "\n";
    //}
    
    std::vector<int> test_val(K,0);
    std::vector<int> miss_val(K,0);
    std::vector<int> test_sum_val(K,0);  
    // monitoring the cluster sizes 
    std::vector<int> cluster_sizes(K, 0);
    int j = 0;
    // Tracking changes for histogram/cluster reassignment
    std::vector<std::tuple<size_t, size_t, size_t>> histogram_changes;
    std::vector<std::tuple<size_t, size_t, size_t>> prev_histogram_changes;
    // Storing most distant data to clusters
    std::vector<std::pair<size_t, double>> clusters_empty;
    std::vector<std::pair<double, size_t>> most_distant(K, {0.0, 0}); 
    //Main Loop
    while (std::abs(sdth_old - sdth) > 0.00001 && j < max_iter){
        #ifdef DEBUG_BASE
            if (j % 1 == 0){
                std::cout << "\nin progress... " << j << "\n";
            }
        #endif
        #ifdef DEBUG_DESPERATION
            for (size_t idx=0; idx < histograms.size(); ++idx)
                histograms[idx].debug_print();
        #endif
        sdth_old = sdth;
        labels_prev = labels_cur;
        sumd_prev = sumd;
        // tuple of histogram changes  [(data_idx, old_cluster, new_cluster), ...]
        clusters_empty.clear();
        std::fill(miss_val.begin(), miss_val.end(), 0);
        std::fill(test_sum_val.begin(), test_sum_val.end(), 0);
        std::fill(cluster_sizes.begin(), cluster_sizes.end(), 0);
        //4.1 Compute distances & labels
        std::fill(sumd.begin(), sumd.end(), 0);
        std::fill(most_distant.begin(), most_distant.end(), std::make_pair(0.0, 0));
        
        sdth = 0; 
        for (size_t i = 0; i < data.size(); i++) {
            size_t best_cluster; 
            size_t curr_cluster = labels_cur[i];
            cluster_sizes[curr_cluster]++ ; // incrementing the cluster counter
            for (auto const& token : data[i]) {
                    test_val = histograms.get_all(token);
                    std::transform(test_val.begin(), test_val.end(),
                        miss_val.begin(),
                        miss_val.begin(),
                        [](int val, int miss) { return miss + (val == 0 ? 1 : 0); }); 
                    std::transform(test_val.begin(), test_val.end(),
                        test_sum_val.begin(),
                        test_sum_val.begin(),
                        [](int val, int sum_val) { return sum_val + val; });
            } // Calculate all distances for data row
            
            std::vector<double> jaccard_distances(hist_sums.begin(), hist_sums.end());
            std::transform(miss_val.begin(), miss_val.end(),
                        jaccard_distances.begin(),
                        jaccard_distances.begin(),
                        [](int miss, int hist_total) { return miss + hist_total; });
            std::transform(test_sum_val.begin(), test_sum_val.end(),
                        jaccard_distances.begin(),
                        jaccard_distances.begin(),
                        [](int tsum, double denom) { return 1 - (tsum / denom); });
            #ifdef DEBUG_DESPERATION
                std::cout << "JDISTS[" << i << "] = ";
                for (auto dist: jaccard_distances)
                    std::cout << std::dec << std::format("{0:0.6f}, ",dist);
                std::cout << "\n";
            #endif
            /*std::cout << "ClusterFor[" << i << "] = " << labels_cur[i] << "\n";
            for (size_t j = 0; j < K; j++ ){
                std::cout << "SUM[ n(y)*d_1 ]: " << test_sum_val[j] 
                          << " , SUM y(n): " << hist_sums[j] 
                          << " , d_2: " << miss_val[j] <<  "\n";
                std::cout << "Jaccard[" << j << "] = " << jaccard_distances[j] << "\n"; 
            }*/
            for (size_t c = 0; c < K; ++c) {
                if (jaccard_distances[c] > most_distant[c].first) {
                    most_distant[c] = { jaccard_distances[c], i };
                }
            }
            best_cluster = std::distance(jaccard_distances.begin(),
                                         std::min_element(jaccard_distances.begin(), jaccard_distances.end()));
            #ifdef DEBUG_DESPERATION
                std::cout << std::format("Best: {0} => {1:0.6f}\n", best_cluster, jaccard_distances[best_cluster] );
            #endif
            //std::cout << "Cluster " << curr_cluster << " -> " << best_cluster << ": Ddelta=" << jaccard_distances[curr_cluster] - jaccard_distances[best_cluster] << "\n";
            auto selection = curr_cluster;
            if ((j == 0) || ((jaccard_distances[curr_cluster] - jaccard_distances[best_cluster]) > 0.0 )){
                cluster_sizes[curr_cluster]--;  // decrement old cluster
                cluster_sizes[best_cluster]++;
                if (cluster_sizes[curr_cluster] == 0)
                    clusters_empty.emplace_back(curr_cluster, jaccard_distances[curr_cluster]);
                histogram_changes.emplace_back(i,curr_cluster, best_cluster);
                selection = best_cluster;
            }
            labels_cur[i] = selection;
            sumd[selection] += jaccard_distances[selection];
            #ifdef DEBUG_DESPERATION
                std::cout << std::format("SUMD[BEST] = {0:0.6f}\n", sumd[selection]);
            #endif
            sdth += jaccard_distances[selection];    
            
            
            std::fill(miss_val.begin(), miss_val.end(), 0);
            std::fill(test_sum_val.begin(), test_sum_val.end(), 0);
            test_val.clear();
        } // Iteration over Data rows
        #ifdef DEBUG_DESPERATION
            std::cout << std::format("SUMD = ");
            for (auto ds: sumd)
                std::cout << std::format("{0:0.6f}, ", ds);
            std::cout << "\n";
            std::cout << std::format("SDTH = {0:0.6f}\n",sdth);
        #endif
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

        //4.2 Stop if error increases
        if (sdth > sdth_old) {
            std::cout << std::format("Error Increased: sdth={0:0.6f} > sdth_old={1:0.6f}\n", sdth, sdth_old);
            sumd = sumd_prev;
            //reverse histogram changes
            for (auto it = prev_histogram_changes.rbegin(); it != prev_histogram_changes.rend(); ++it){
                size_t row_idx, old_cluster, new_cluster;
                std::tie(row_idx, old_cluster, new_cluster) = *it;
                labels_cur[row_idx] = old_cluster;
                for (auto const& token : data[row_idx]) {
                    histograms.decrement(new_cluster, token);
                    if ( j!= 0)
                        histograms.increment(old_cluster, token);
                }
            } 
            //histograms = hist_prev;
            break;
        }
        if (j==0){
            histograms.zero_out();
        } 
        //4.3. Recompute histograms by applying histogram changes using data rows
        for (auto const& change : histogram_changes){
            size_t row_idx, old_cluster, new_cluster;
            std::tie(row_idx, old_cluster, new_cluster) = change;
            //labels_cur[row_idx] = new_cluster;
            #ifdef DEBUG_DESPERATION
                std::cout << std::format("D_change[{0:d}] = {1:d} -> {2:d}\n", row_idx, old_cluster, new_cluster);
            #endif
            for (auto const& token : data[row_idx]) {
                if (j != 0)
                    histograms.decrement(old_cluster, token);
                histograms.increment(new_cluster, token);
            }
        }
        /*for (size_t i = 0; i < histograms.size(); i++){
            std::cout << "Cluster Histogram:" << i << "\n";
            histograms[i].debug_print();
        }
        */
        prev_histogram_changes.swap(histogram_changes);
        histogram_changes.clear();
        hist_sums = histograms.totals();
        //4.4 increment counter
        #ifdef DEBUG_BASE
            for (size_t idx=0; idx < histograms.size(); ++idx)
                histograms[idx].debug_print();
        #endif
        ++j;
    }
    // 5. Results


    auto histogram_out = to_hist_output_format(histograms, max_hist_length);

    return {labels_cur, histogram_out, sumd};
}


}





    

