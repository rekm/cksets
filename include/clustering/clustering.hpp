
#pragma once
#include <string>
#include <unordered_map> 
#include <hashing/murmur_hash.hpp>

using Histogram = std::unordered_map<std::string, int, ksets::MurmurHash>;

namespace ksets
{
    struct ClusterResult {
        std::vector<int> labels;
        std::vector<std::vector<int>> histograms;
        std::vector<double> sumd;
    };

    ClusterResult cluster_cpp(
            const std::vector<std::vector<std::string>> & data,
            int K,
            int max_hist_length,
            std::vector<std::vector<int>> histograms = {},
            int max_iter = 50);
}




    

