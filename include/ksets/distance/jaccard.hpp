#pragma once 

#include <ksets/distance/base.hpp>

namespace ksets::distance {
    template<class HistogramCollection>
    class JaccardDistance
        : public DistancePolicy<JaccardDistance<HistogramCollection>, HistogramCollection>{
        using Base = DistancePolicy<JaccardDistance, HistogramCollection>;

        std::vector<int> miss_;
        std::vector<hist_count_t> tsum_;
        std::vector<double> dists_;
        std::vector<hist_count_t> scratch_;
        std::vector<hist_count_t> hist_sums_;
        
        public:
            using Base::Base;
            using Base::H_;

            void begin_iteration_impl(size_t iter){
                hist_sums_ = H_.totals();
            }

            void begin_row_impl() {
                miss_.assign(H_.size(), 0);
                tsum_.assign(H_.size(), 0);
                dists_.assign(H_.size(), 0.0);
                scratch_.resize(H_.size());
            }

            template <typename Range>
            void accumulate_token_impl(const token_t& token, const Range& range) {
                /*
                Transform code actually as fast as the loop below
                std::fill(test_val.begin(), test_val.end(), 0);
                    histograms.get_all(token, test_val, histRange);
                    std::transform(test_val.begin(), test_val.end(),
                        miss_val.begin(),
                        miss_val.begin(),
                        [](int val, int miss) { return miss + (val == 0 ? 1 : 0); }); 
                    std::transform(test_val.begin(), test_val.end(),
                        test_sum_val.begin(),
                        test_sum_val.begin(),
                        [](int val, int sum_val) { return sum_val + val; });*/
                std::fill(scratch_.begin(), scratch_.end(), 0);
                H_.get_all(token, scratch_, range);

                for (size_t k=0;k<scratch_.size();++k){
                    auto v = scratch_[k];
                    miss_[k] += (v == 0);
                    tsum_[k] += v;
                }
            }
            /*
                        m
                        ___
                        \   n(y_i) * δ1(y_i)
                        /__
                        i = 1
            d_J = 1 - --------------------
                        m
                        ___
                        \   n(y_i) + δ2(x_i)
                        /__
                        i = 1
            */

            const std::vector<double>& finalize_impl() {
                /*
                Old transform version - Was also not the problem
                std::vector<double> jaccard_distances(hist_sums.begin(), hist_sums.end());
                std::transform(miss_val.begin(), miss_val.end(),
                        jaccard_distances.begin(),
                        jaccard_distances.begin(),
                        [](int miss, int hist_total) { return miss + hist_total; });
                std::transform(test_sum_val.begin(), test_sum_val.end(),
                        jaccard_distances.begin(),
                        jaccard_distances.begin(),
                        [](int tsum, double denom) { return 1 - (tsum / denom); });
                */
                for (size_t k=0;k<scratch_.size();++k){
                    auto denom = miss_[k] + hist_sums_[k];
                    dists_[k] = 1.0 - double(tsum_[k]) / denom;
                }
                return dists_;
            }
            /*std::cout << "ClusterFor[" << i << "] = " << labels_cur[i] << "\n";
            for (size_t j = 0; j < K; j++ ){
                std::cout << "SUM[ n(y)*d_1 ]: " << test_sum_val[j] 
                          << " , SUM y(n): " << hist_sums[j] 
                          << " , d_2: " << miss_val[j] <<  "\n";
                std::cout << "Jaccard[" << j << "] = " << jaccard_distances[j] << "\n"; 
            }*/


    };

} // ksets::distance