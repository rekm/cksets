
#pragma once
#include <ksets/histogram/collection_base.hpp>
#include <ksets/clustering/histogram_update_policy.hpp>
#include <ksets/clustering/histogram_preselect_policy.hpp>
#include <ksets/hashing/murmur_hash.hpp>
#include <ksets/clustering/clustering.hpp>
#include <ksets/distance/jaccard.hpp>
#include <ksets/types.hpp>
#include <string> 
#include <format>
#include <tuple>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <ranges>
#include <vector>
//using Histogram = std::unordered_map<std::string, int, ksets::MurmurHash>;

namespace ksets
{
    
struct ClusterResult {
    std::vector<label_t> labels;
    std::vector<std::vector<hist_count_t>> histograms;
    std::vector<double> sumd;
};

template <class HistogramCollection>
std::vector<std::vector<hist_count_t>> to_hist_output_format(
    const HistogramCollection& hist_vec,
    int max_hist_length
){
    std::vector<std::vector<hist_count_t>> out;
    out.reserve(hist_vec.size());

    for (size_t i = 0; i < hist_vec.size(); ++i) {
        out.push_back(hist_vec[i].values(max_hist_length));
    }
    return out;
}

template <class HistogramCollection, 
          template<class> class HistogramUpdatePolicy,
          template<class> class HistogramPreselectPolicy>
ClusterResult cluster_cpp(
        const std::vector<std::vector<token_t>> & data,
        int K,
        int max_hist_length,
        HistogramCollection& histograms,
        int max_iter = 50, 
        int seed = 42){
    bool histograms_provided = ! histograms.empty();
    HistogramPreselectPolicy<HistogramCollection> histPreselector(histograms, data);
    HistogramUpdatePolicy<HistogramCollection> histUpdater(histograms, data, histograms_provided);
    if (max_iter == 0)
        max_iter = 50;

    std::vector<label_t> labels_cur(data.size());
    std::vector<label_t> labels_prev(data.size());
    // Build initial histograms
    auto randomizer = ksets::hashing::HashPolicy<ksets::hashing::MurmurHash>(ksets::hashing::MurmurHash(seed));
    
    if (! histograms_provided){
        for (size_t i = 0; i < data.size(); i++) {
            size_t cluster = randomizer(i) % K;
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
    std::vector<hist_count_t> hist_sums = histograms.totals();
    /*for (size_t i = 0; i < hist_sums.size(); ++i) {
        std::cout << "Cluster " << i << " total: " << hist_sums[i] << "\n";
    }*/
    
    std::vector<hist_count_t> test_val(K,0);
    std::vector<int> miss_val(K,0);
    std::vector<int> test_sum_val(K,0);  
    // monitoring the cluster sizes 
    std::vector<int> cluster_sizes(K, 0);
    int j = 0;
    
    // Storing most distant data to clusters
    std::vector<std::pair<size_t, double>> clusters_empty;
    std::vector<std::pair<double, size_t>> most_distant(K, {0.0, 0}); 
    //Main Loop
    while ((std::abs(sdth_old - sdth) > 0.00001 ) && (j < max_iter)){
        
        histUpdater.before_iteration(j);
        histPreselector.before_iteration(j);
        sdth_old = sdth;
        labels_prev = labels_cur;
        sumd_prev = sumd;
        
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
            auto histRange = histPreselector.get_candidate_histograms(data[i]);
            for (auto const& token : data[i]) {
                    std::fill(test_val.begin(), test_val.end(), 0);
                    histograms.get_all(token, test_val, histRange);
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
                histUpdater.register_move(i,curr_cluster, best_cluster);
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
            //test_val.clear();
        } // Iteration over Data rows
        #ifdef DEBUG_DESPERATION
            std::cout << std::format("SUMD = ");
            for (auto ds: sumd)
                std::cout << std::format("{0:0.6f}, ", ds);
            std::cout << "\n";
            std::cout << std::format("SDTH = {0:0.6f} , SDTH_OLD = {1:0.6f} \n",sdth, sdth_old);
        #endif
        

        //4.2 Stop if error increases
        if (sdth > sdth_old) {
            //std::cout << std::format("Error Increased: sdth={0:0.6f} > sdth_old={1:0.6f}\n", sdth, sdth_old);
            sumd = sumd_prev;
            //reverse histogram changes
            histUpdater.rollback(labels_cur);
            //histograms = hist_prev; is ksets equivalent
            break;
        }
        if (j==0 && histograms_provided){
            histograms.zero_out();
        } 
        //4.3. Recompute histograms by applying histogram changes using data rows
        histUpdater.commit(labels_cur);
        histPreselector.update(labels_cur);
        /*for (size_t i = 0; i < histograms.size(); i++){
            std::cout << "Cluster Histogram:" << i << "\n";
            histograms[i].debug_print();
        }*/
        
        
        
        hist_sums = histograms.totals();
        //4.4 increment counter
        #ifdef DEBUG_BASE
            for (size_t idx=0; idx < histograms.size(); ++idx)
                histograms[idx].debug_print();
        #endif
        ++j;
    }
    std::vector<std::size_t> hist_lengths;
    hist_lengths.reserve(histograms.size());
    double avg_hist_len = 0;
    size_t median_hist_len = 0;
    size_t num_hists = histograms.size();
    for (size_t idx=0; idx < num_hists; ++idx) {
        avg_hist_len += histograms[idx].size();
        hist_lengths.push_back(histograms[idx].size()); 
    }
    std::sort(hist_lengths.begin(), hist_lengths.end());
    if (hist_lengths.size() % 2 == 0)
    {
      median_hist_len = (hist_lengths[num_hists / 2 - 1] + hist_lengths[num_hists / 2]) / 2;
    }
    else 
    {
      median_hist_len = hist_lengths[num_hists / 2];
    }
    
    avg_hist_len = avg_hist_len / num_hists; 
    std::cout << "Total Iterations: " << j << "\n"
              << "Avg Histogram Size: " << avg_hist_len << "\n"
              << "Median Histogram Size: " << median_hist_len << "\n"
              << "Sum of Total Distances: " << sdth << "\n"; 
    size_t hist_quant = 1;
    double hist_quant_perc = 0; 
    for (const auto& chunk : hist_lengths | std::views::chunk(std::max(num_hists / 10, size_t{1}) )) {
        auto chunk_avg = std::reduce(chunk.begin(), chunk.end()) / chunk.size();
        double chunk_perc = chunk.size()/ double{num_hists};
        std::cout << "AVG Histogram Size N-Tile " << hist_quant 
                  << "(n=" << chunk.size() << ", " << hist_quant_perc << "-" << std::min(hist_quant_perc + chunk_perc, 1.0) * 100 
                  << "%) = " << chunk_avg << "\n";
        hist_quant_perc += chunk_perc;
        hist_quant++;  
    }
    // 5. Results


    auto histogram_out = to_hist_output_format(histograms, max_hist_length);

    return {labels_cur, histogram_out, sumd};
}


}





    

