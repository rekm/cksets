
#include <vector>
#include <string>
#include <clustering/clustering.hpp>
#include <limits>

namespace ksets
{
    ClusterResult cluster_cpp(
            const std::vector<std::vector<std::string>> & data,
            int K,
            int max_hist_length,
            std::vector<std::vector<int>> histograms,
            int max_iter){

        if (max_iter == 0)
        max_iter = 50;

        // 1. Determine dimensions 

        // 2. Build initial histograms

        // 3. hash_representation

        std::vector<int> labels_prev;
        std::vector<int> labels_cur;
        auto hist_prev = histograms;
        double sdth_old = 0.0;
        double sdth = std::numeric_limits<double>::max();
        std::vector<double> sumd_prev;
        std::vector<double> sumd;

        int j = 0;
        // 4. Main Loop

        // Results

        return {labels_cur, histograms, sumd};
    }
}
