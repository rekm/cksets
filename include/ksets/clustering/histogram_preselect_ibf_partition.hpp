#pragma once
#include <ksets/clustering/histogram_preselect_policy.hpp>
#include <ksets/histogram/collection_iterator.hpp>
#include <ksets/types.hpp> // for token_t, move_t
#include <hibf/interleaved_bloom_filter.hpp> // for bin_index, interleaved_bloom_filter, bin_count, bin_size, hash_...
#include <hibf/misc/print.hpp>               // for print, print_t
#include <hibf/misc/bit_vector.hpp>
#include <hibf/build/bin_size_in_bits.hpp>
#include <ranges>
#include <numeric>
#include <vector>
#include <algorithm>
#include <cmath> 


namespace ksets {

    

    template <typename HistogramCollection>
    class HistPreselectIBFPartitioned
        : public HistogramPreselectPolicy<HistPreselectIBFPartitioned<HistogramCollection>, HistogramCollection>
    {
    public:
        using Base = HistogramPreselectPolicy<HistPreselectIBFPartitioned<HistogramCollection>,
                            HistogramCollection>;
        using Base::Base;
        
        explicit HistPreselectIBFPartitioned(HistogramCollection& hist,
                            const std::vector<std::vector<token_t>>& data, 
                            size_t cluster_limit=1024, double fpr=0.005)
            : Base(hist, data), 
            cluster_limit_(cluster_limit),  
            hists_(hist), data_(data), indices_(hist.size()),
            fpr_(fpr)
        {
            num_partitions_ = (hists_.size() + cluster_limit_ - 1) / cluster_limit_;
            all_bins_.resize(num_partitions_);
            for (size_t i = 0; i < hists_.size(); ++i){
                size_t p = i / cluster_limit_;
                size_t prev = p * cluster_limit_;
                all_bins_[p].push_back(seqan::hibf::bin_index{i - prev});
            }
            
            for (auto & part : all_bins_) {
                ibf_sizes_.push_back(part.size());
            }    
            ibfs_ = make_ibfs(hist, ibf_sizes_, 3u, fpr);
            std::iota(indices_.begin(), indices_.end(), 0);
            std::iota(fallback_indices_.begin(), fallback_indices_.end(), 0);                        
            cluster_agents_.resize(num_partitions_);
            for (size_t i = 0; i<num_partitions_ ; ++i)
                cluster_agents_[i] = ibfs_[i].membership_agent();
        }

        __attribute__((noinline)) void before_iteration_impl(size_t it_idx) {
            if(it_idx == 0){
                for (size_t i = 0; i < hists_.size(); ++i) {
                    auto coords = local_cluster(i);
                    auto & hist = hists_[i];
                    auto & ibf = ibfs_[coords.first];
                    for (const auto& key : hist.keys())
                        ibf.emplace(key, seqan::hibf::bin_index{coords.second});
                        //ibfs_[coords.first].emplace(key, seqan::hibf::bin_index{coords.second});
                }
                for (size_t i = 0; i<num_partitions_ ; ++i)
                    cluster_agents_[i] = ibfs_[i].membership_agent();
            }
        }

        auto get_candidate_histograms_impl(const std::vector<token_t>& data_row) {
            //uint16_t threshold = compute_threshold(thresh_mode_, data_row.size());
            //    static_cast<uint16_t>(data_row.size() * fpr_ + 0.999999);
            uint16_t threshold =  static_cast<uint16_t>(0.9999 + std::sqrt(data_row.size()) * .1);
            //uint16_t threshold =  static_cast<uint16_t>(0.9999 + std::sqrt(data_row.size()) * 1.0);
            //uint16_t threshold =  static_cast<uint16_t>(0.9999 + data_row.size() * 0.5);

            //return hists_.get_range(cluster_agent_.membership_for(data_row, threshold));
            size_t prev = 0; 
            size_t write_pos = 0;
            for (size_t ibf_idx = 0; ibf_idx < ibfs_.size(); ++ibf_idx){
                 
                auto const& res = cluster_agents_[ibf_idx].membership_for(data_row, threshold);
                if (res.empty()){
                    prev += cluster_limit_;
                    continue;
                }
                
                for (size_t i=0; i < res.size(); ++i) {
                    indices_[i + write_pos] = res[i] + prev;
                }
                // Needs prev needs to be added to the res and written into indices 
                prev += cluster_limit_;
                write_pos += res.size();  
            } 
            
            //indices_.clear();
            //indices_.insert(indices_.end(), res.begin(), res.end());
            if (write_pos == 0){
                indices_.assign(fallback_indices_.begin(), fallback_indices_.end());
                write_pos = fallback_indices_.size();
            }
                
            //} else {
            //    indices_.assign(res.begin(), res.end());
                //indices_ = cluster_agent_.membership_for(data_row, threshold);
            //}
            
            return hists_.get_range(std::span{indices_.begin(), write_pos});
            //return hists_.get_range(res);
        }


        void update_impl(std::vector<label_t>& labels_cur) {
            for (size_t i=0; i<ibfs_.size(); ++i)
                ibfs_[i].clear(all_bins_[i]);
            before_iteration_impl(0);
            
        }

        void update_from_changes(std::vector<Change>& changes){


        } 

        enum class ThresholdMode {
            Linear,
            SqrtLow,
            SqrtHigh
        };

    private:
        HistogramCollection& hists_;
        size_t cluster_limit_ = 512;
        size_t num_partitions_ = 1;
        ThresholdMode  thresh_mode_ =  ThresholdMode::SqrtLow; 
        const std::vector<std::vector<token_t>>& data_;
        std::vector<size_t> ibf_sizes_;
        std::vector<size_t> indices_;
        std::vector<size_t> fallback_indices_;
        double fpr_ = 0.01;
        std::vector<std::vector<seqan::hibf::bin_index>> all_bins_;
        std::vector<seqan::hibf::interleaved_bloom_filter> ibfs_;
        std::vector<seqan::hibf::interleaved_bloom_filter::membership_agent_type> cluster_agents_;
        
        static size_t estimate_maxhist_elems(HistogramCollection& hists){
            size_t max_elements = 1;
            long double avg_elements = 0.0;
        
            for (size_t idx=0; idx< hists.size(); idx++) {
                max_elements = std::max<size_t>(max_elements, hists[idx].size());
                avg_elements +=  static_cast<long double>(hists[idx].size());
            }
            if (hists.size() < 1U)
                throw std::invalid_argument("Cannot build IBF from empty histogram collection");
            avg_elements = avg_elements / static_cast<long double>(hists.size());

            size_t max_expected = std::ceil(avg_elements * (std::log(static_cast<long double>(hists.size())) + 0.5772156649015328606L)); // Euler-Mascheroni Constant
            return std::max(max_expected, max_elements);
        }

        inline uint16_t compute_threshold(ThresholdMode mode, size_t n) {
            switch (mode) {
                case ThresholdMode::Linear:
                    return static_cast<uint16_t>(0.9999 + n * 0.5);
                case ThresholdMode::SqrtLow:
                    return static_cast<uint16_t>(0.9999 + std::sqrt(n) * 0.1);
                case ThresholdMode::SqrtHigh:
                    return static_cast<uint16_t>(0.9999 + std::sqrt(n) * 1.0);
            }
            __builtin_unreachable();
        }

        std::pair<size_t, size_t> local_cluster(size_t cluster_idx) const {
            return {cluster_idx / cluster_limit_, cluster_idx % cluster_limit_}; 
        } 

        static auto make_ibfs(HistogramCollection& hists,
                const std::vector<size_t> & ibf_sizes, size_t hash_fun_count=3, double fpr=0.01){

              size_t max_elems_expected = estimate_maxhist_elems(hists); 
              std::vector<seqan::hibf::interleaved_bloom_filter> out;
              size_t num_ibfs = ibf_sizes.size();
              if (num_ibfs < 1U )
                    throw std::invalid_argument("ibf_sizes needs to be a vector that contains at least one element!!");
              out.reserve(num_ibfs);
              for (size_t i=0; i < num_ibfs; ++i){
                out.emplace_back(
                    make_ibf(max_elems_expected, 
                             ibf_sizes[i], 
                             hash_fun_count, 
                             fpr)
                    );
              }

              return out;
        }

        

        static auto make_ibf(
                size_t max_elems, 
                size_t ibf_size, 
                size_t hash_fun_count=3, 
                double fpr=0.01){

        
        seqan::hibf::build::bin_size_parameters params{};
        params.elements   = 4 * max_elems;
        params.hash_count = hash_fun_count;
        params.fpr        = fpr;

        return seqan::hibf::interleaved_bloom_filter{
            seqan::hibf::bin_count{ibf_size},
            seqan::hibf::bin_size{
                seqan::hibf::build::bin_size_in_bits(params)},
            seqan::hibf::hash_function_count{hash_fun_count}};
        }
    };

} // namespace ksets
