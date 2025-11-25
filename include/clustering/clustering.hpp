
#pragma once
#include <string>
#include <unordered_map> 
#include <memory/pool_allocator.hpp>
#include <histogram/map_histogram.hpp>
#include <histogram/base_histogram_collection.hpp>
#include <hashing/hash_policy.hpp>
#include <hashing/murmur_hash.hpp>
#include <string> 
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
    int j = 0;
    while (std::abs(sdth_old - sdth) > 0.00001 && j < max_iter){
        sdth_old = sdth;
        labels_prev = labels_cur;
        sumd_prev = sumd;
        std::vector<int> test_val(K,0);
        std::vector<int> miss_val(K,0);
        std::vector<int> test_sum_val(K,0);  
        //4.1 Compute distances & labels
        for (size_t i = 0; i < data.size(); i++) {
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
                    
                    //d_miss += yval == 0;
                    //y_sum += yval; 
            }
            for (size_t j = 0; j < K; j++ ){
                std::cout << " , SUM[ n(y)*d_1 ]: " << test_sum_val[j] << " , SUM y(n): " << hist_sums[j] << " , d_2: " << miss_val[j] <<  "\n"; 

            }
            
            std::fill(miss_val.begin(), miss_val.end(), 0);
            std::fill(test_sum_val.begin(), test_sum_val.end(), 0);
            /*
            for (size_t j = 0; j < K; j++){
                //auto cluster = labels_prev[i];
                int d_miss = 0;
                int y_sum = 0;
                for (auto const& token : data[i]) {
                    int yval = histograms[j][token];
                    std::cout << token << ": " << yval << ", "; 
                    d_miss += yval == 0;
                    y_sum += yval; 
                }
                std::cout << ">> " << "y_hit: " << y_sum << ", d_miss: " << d_miss <<  "\n";
            }
            std::cout << "<>\n";
            */
            
        }
        //4.2 Stop if error increases
        if (sdth > sdth_old) {
            labels_cur = labels_prev;
            sumd = sumd_prev;
            //histograms = hist_prev;
            break;
        }
        histograms[0].debug_print();
        //4.3. Recompute histograms

        //4.4 increment counter
        ++j;
    }
    // 5. Results


    auto histogram_out = to_hist_output_format(histograms, max_hist_length);

    return {labels_cur, histogram_out, sumd};
}   
}





    

