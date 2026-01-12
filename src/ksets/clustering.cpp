
#include <vector>
#include <string>
#include <limits>
#include <ksets/clustering/clustering.hpp>
#include <string> 
#include <iostream> 


namespace ksets
{
    /*
    //std::vector<std::vector<int>> to_hist_output_format(std::vector<Histogram> const &hist_vec,int max_hist_length);
    template <class HistogramCollection>
    std::vector<std::vector<int>> to_hist_output_format(const HistogramCollection& hist_vec, int max_hist_length);

    template <class HistogramCollection>
    ClusterResult cluster_cpp(
            const std::vector<std::vector<std::string>> & data,
            int K,
            int max_hist_length,
            HistogramCollection& histograms,
            int max_iter){
        
        std::vector<std::vector<int>> histogram_out;
        
        if (max_iter == 0)
            max_iter = 50;
        max_iter = 1;

        std::vector<int> labels_cur(data.size());
        std::vector<int> labels_prev(data.size());
        // 2. Build initial histograms
        for (size_t i = 0; i < data.size(); i++) {
            int cluster = i % K;
            labels_cur[i] = cluster;

            for (auto const& token : data[i]) {
                histograms.increment(cluster,token);  
            }
        }
        // 3. hash_representation

        
        auto hist_prev = histograms;
        double sdth_old = 0.0;
        double sdth = std::numeric_limits<double>::max();
        std::vector<double> sumd_prev;
        std::vector<double> sumd;
        /*
        
        int j = 0;
        // 4. Main Loop
        while (std::abs(sdth_old - sdth) > 0.00001 && j < max_iter){
            sdth_old = sdth;
            labels_prev = labels_cur;
            sumd_prev = sumd;
            std::vector<int> test_val(K); 
            //4.1 Compute distances & labels
            for (size_t i = 0; i < data.size(); i++) {
                for (size_t j = 0; j < K; j++){
                    //auto cluster = labels_prev[i];
                    int d_miss = 0;
                    int y_sum = 0;
                    for (auto const& token : data[i]) {
                        int yval = histogram_vec[j][token];
                        std::cout << token << ": " << yval << ", "; 
                        d_miss += yval == 0;
                        y_sum += yval; 
                    }
                    std::cout << ">> " << "y_hit: " << y_sum << ", d_miss: " << d_miss <<  "\n";
                }
                std::cout << "<>\n";
            }
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
        
        histogram_out = to_hist_output_format(histograms, max_hist_length);
        return {labels_cur, histogram_out, sumd};
    }

    template <typename HistogramCollection>
    std::vector<std::vector<int>> to_hist_output_format(
            const HistogramCollection& hist_vec,
            int max_hist_length){
        std::vector<std::vector<int>> out;
        out.reserve(hist_vec.size());

        for (size_t i = 0; i < hist_vec.size(); ++i) {
            out.push_back(hist_vec[i].values(max_hist_length));
        }
        return out;
    }
*/
}
