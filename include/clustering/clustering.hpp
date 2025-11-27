
#pragma once
#include <histogram/base_histogram_collection.hpp>
#include <hashing/hash_policy.hpp>
#include <hashing/murmur_hash.hpp>
#include <string> 
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
    int max_iter = 50
){
    if (max_iter == 0)
        max_iter = 50;

    std::vector<int> labels_cur(data.size());
    std::vector<int> labels_prev(data.size());
    // Build initial histograms
    for (size_t i = 0; i < data.size(); i++) {
        int cluster = i % K;
        labels_cur[i] = cluster;
        for (auto const& token : data[i]) {
            histograms.increment(cluster, token);
        }
    }

    double sdth_old = 0.0;
    double sdth = std::numeric_limits<double>::max();
    std::vector<double> sumd_prev;
    std::vector<double> sumd;
    std::vector<int> hist_sums = histograms.totals();
    for (size_t i = 0; i < hist_sums.size(); ++i) {
        std::cout << "Cluster " << i << " total: " << hist_sums[i] << "\n";
    }

    std::vector<int> test_val(K,0);
    std::vector<int> miss_val(K,0);
    std::vector<int> test_sum_val(K,0);  

    int j = 0;
    std::vector<std::tuple<size_t, size_t, size_t>> histogram_changes;
    std::vector<std::tuple<size_t, size_t, size_t>> prev_histogram_changes; 
    while (std::abs(sdth_old - sdth) > 0.00001 && j < max_iter){
        sdth_old = sdth;
        labels_prev = labels_cur;
        sumd_prev = sumd;
        // tuple of histogram changes  [(data_idx, old_cluster, new_cluster), ...]
        std::fill(miss_val.begin(), miss_val.end(), 0);
        std::fill(test_sum_val.begin(), test_sum_val.end(), 0);
        //4.1 Compute distances & labels
        sdth = 0; 
        for (size_t i = 0; i < data.size(); i++) {
            size_t best_cluster; 
            size_t curr_cluster = labels_cur[i];
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
            }
            
            std::vector<double> jaccard_distances(hist_sums.begin(), hist_sums.end());
            //jaccard_distances.swap(hist_sums);
            std::transform(miss_val.begin(), miss_val.end(),
                        jaccard_distances.begin(),
                        jaccard_distances.begin(),
                        [](int miss, int hist_total) { return miss + hist_total; });
            std::transform(test_sum_val.begin(), test_sum_val.end(),
                        jaccard_distances.begin(),
                        jaccard_distances.begin(),
                        [](int tsum, double quotient) { return tsum / quotient; });
            /*std::cout << "ClusterFor[" << i << "] = " << labels_cur[i] << "\n";
            for (size_t j = 0; j < K; j++ ){
                std::cout << "SUM[ n(y)*d_1 ]: " << test_sum_val[j] 
                          << " , SUM y(n): " << hist_sums[j] 
                          << " , d_2: " << miss_val[j] <<  "\n";
                std::cout << "Jaccard[" << j << "] = " << jaccard_distances[j] << "\n"; 
            }*/
            best_cluster = std::distance(jaccard_distances.begin(),
                                         std::max_element(jaccard_distances.begin(), jaccard_distances.end()));
            if (best_cluster != curr_cluster){
                std::cout << "Cluster " << curr_cluster << " -> " << best_cluster << ": Ddelta=" << jaccard_distances[curr_cluster] - jaccard_distances[best_cluster] << "\n";
                histogram_changes.emplace_back(i,curr_cluster, best_cluster);
            }
            sdth += jaccard_distances[best_cluster];
            std::fill(miss_val.begin(), miss_val.end(), 0);
            std::fill(test_sum_val.begin(), test_sum_val.end(), 0);
        }
        //4.2 Stop if error increases
        if (sdth > sdth_old) {
            sumd = sumd_prev;
            //reverse histogram changes
            for (auto it = prev_histogram_changes.rbegin(); it != prev_histogram_changes.rend(); ++it){
                size_t row_idx, old_cluster, new_cluster;
                std::tie(row_idx, old_cluster, new_cluster) = *it;
                //labels_cur[row_idx] = old_cluster;
                for (auto const& token : data[row_idx]) {
                    histograms.decrement(new_cluster, token);
                    histograms.increment(old_cluster, token);
                }
            } 
            //histograms = hist_prev;
            break;
        }

        //4.3. Recompute histograms by applying histogram changes using data rows
        for (auto const& change : histogram_changes){
            size_t row_idx, old_cluster, new_cluster;
            std::tie(row_idx, old_cluster, new_cluster) = change;
            labels_cur[row_idx] = new_cluster;
            for (auto const& token : data[row_idx]) {
                histograms.decrement(old_cluster, token);
                histograms.increment(new_cluster, token);
            }
        }
        std::cout << "End of Iteration: " << j << "\n";
        for (size_t i = 0; i < histograms.size(); i++){
            std::cout << "Cluster Histogram:" << i << "\n";
            histograms[i].debug_print();
        }
        prev_histogram_changes.swap(histogram_changes);
        histogram_changes.clear();
        hist_sums = histograms.totals();
        //4.4 increment counter
        
        ++j;
    }
    // 5. Results


    auto histogram_out = to_hist_output_format(histograms, max_hist_length);

    return {labels_cur, histogram_out, sumd};
}   
}





    

