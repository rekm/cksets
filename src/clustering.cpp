
#include <vector>
#include <string>
#include <limits>
#include <clustering/clustering.hpp>

namespace ksets
{
    std::vector<std::vector<int>> to_hist_output_format(std::vector<Histogram> const &hist_vec,int max_hist_length);
    ClusterResult cluster_cpp(
            const std::vector<std::vector<std::string>> & data,
            int K,
            int max_hist_length,
            std::vector<std::vector<int>> histograms,
            int max_iter){
        
        auto seed = 12345;
        std::vector<Histogram> histogram_vec(K, Histogram{0, MurmurHash(seed)});
        
        if (max_iter == 0)
            max_iter = 50;

        // 2. Build initial histograms
        for (size_t i = 0; i < data.size(); i++) {
            int cluster = i % K;

            for (auto const& token : data[i]) {
                histogram_vec[cluster][token]++;  
            }
        }
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
        while (std::abs(sdth_old - sdth) > 0.00001 && j < max_iter){
            sdth_old = sdth;
            labels_prev = labels_cur;
            sumd_prev = sumd;
            //4.1 Compute distances & labels
            
            //4.2 Stop if error increases
            if (sdth > sdth_old) {
                labels_cur = labels_prev;
                sumd = sumd_prev;
                histograms = hist_prev;
                break;
            }
            
            //4.3. Recompute histograms

            //4.4 increment counter
            ++j;
        }
        // 5. Results
        histograms = to_hist_output_format(histogram_vec, max_hist_length);
        return {labels_cur, histograms, sumd};
    }

    std::vector<std::vector<int>> to_hist_output_format(
            std::vector<Histogram> const &hist_vec,
            int max_hist_length){
        std::vector<std::vector<int>> out;
        out.reserve(hist_vec.size());

        for (auto const &hist : hist_vec) {
            std::vector<int> hvec;
            hvec.reserve(max_hist_length);

            for (auto const &kv : hist) {
                hvec.push_back(kv.second);
                if (hvec.size() == max_hist_length)
                    break;
            }
            out.push_back(std::move(hvec));
        }
        return out;
    }
}
