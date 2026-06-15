#pragma once
#include <ksets/clustering/histogram_preselect_policy.hpp>
#include <ksets/histogram/collection_iterator.hpp>
#include <ksets/histogram/utils.hpp>
#include <ksets/preselect/ibf_utils.hpp>
#include <ksets/types.hpp> // for token_t, move_t
#include <hibf/interleaved_bloom_filter.hpp> // for bin_index, interleaved_bloom_filter, bin_count, bin_size, hash_...

#include <numeric>
#include <vector>
#include <algorithm>
#include <cmath> 
#include <cstdint>
#include <type_traits>


namespace ksets {

    

    template <typename HistogramCollection, typename Distance, class ValueType = std::uint16_t>
    class HistPreselectIBF
        : public HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection, Distance>, HistogramCollection, Distance>
    {
        using ThresholdPolicy =
        typename preselect::ThresholdPolicyFor<
            HistogramCollection,
            Distance
        >::type;

        ThresholdPolicy threshold_policy_;

        static constexpr bool dynamic_threshold_ =
            preselect::threshold_policy_traits<ThresholdPolicy>::dynamic_threshold;

        using agent_type =
            std::conditional_t<dynamic_threshold_,
                typename seqan::hibf::interleaved_bloom_filter::template counting_agent_type<ValueType>,
                typename seqan::hibf::interleaved_bloom_filter::membership_agent_type>;    
    public:
        using Base = HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection, Distance>,
                            HistogramCollection, Distance>;
        using Base::Base;

        
        
        explicit HistPreselectIBF(HistogramCollection& hist,
                            const std::vector<std::vector<token_t>>& data, Distance& dist, double fpr=0.005)
            : Base(hist, data, dist),  
            hists_(hist), data_(data), 
            threshold_policy_(hist, dist),
            indices_(hist.size()), dist_(dist),
            fpr_(fpr), 
            ibf_(preselect::make_ibf(histogram::estimate_maxhist_elems(hist), hist.size(), 3u, fpr))
        {
            std::iota(indices_.begin(), indices_.end(), 0);
            std::iota(fallback_indices_.begin(), fallback_indices_.end(), 0);
            all_bins_.reserve(ibf_.bin_count());
            if constexpr (dynamic_threshold_)
                precomputed_distances_.resize(data_.size());
            for (size_t i = 0; i < ibf_.bin_count(); ++i)
                all_bins_.push_back(seqan::hibf::bin_index{i});
            refresh_agent();
        }
        
        void refresh_agent(){
            if constexpr (dynamic_threshold_)
                cluster_agent_ = ibf_.counting_agent();
            else
                cluster_agent_ = ibf_.membership_agent();
        }

        void before_iteration_impl(size_t it_idx, std::vector<label_t>& labels_cur) {
            threshold_policy_.before_iteration(it_idx);
            if constexpr (dynamic_threshold_)
                precompute_distances(labels_cur);
            if(it_idx == 0){
                fill_ibf();
                refresh_agent();
            }
        }

        auto get_candidate_histograms_impl(const std::vector<token_t>& data_row, size_t row_idx) {
            //uint16_t threshold =
            //    static_cast<uint16_t>(data_row.size() * fpr_ + 0.999999);
            if constexpr (dynamic_threshold_)
            {
                //possibly need to check if the largest possible distance is smaller than dist for any 
                //of  the returned counts. 
                dist_base_t dist = precomputed_distances_[row_idx];
                auto const& thresholds = threshold_policy_.compute_min_thresholds(dist, data_row.size());
                preselect::membership_with_dynamic_min_threshold(data_row, indices_, thresholds, cluster_agent_);
                if (indices_.empty())
                    indices_.assign(fallback_indices_.begin(), fallback_indices_.end());
                // Code using ibf_.counting_agent() and storing the index results in res buffer 
            }
            else
            {
                uint16_t threshold =  threshold_policy_.compute_threshold(data_row.size());
                auto const& res = cluster_agent_.membership_for(data_row, threshold);
                if (res.empty()){
                    indices_.assign(fallback_indices_.begin(), fallback_indices_.end());
                } else {
                    indices_.assign(res.begin(), res.end());
                    //indices_ = cluster_agent_.membership_for(data_row, threshold);
                }
            }
            
            
            //indices_.clear();
            //indices_.insert(indices_.end(), res.begin(), res.end());
            
            
            return hists_.get_range(indices_);
            //return hists_.get_range(res);
        }

        

        void update_impl(std::vector<label_t>& labels_cur) {
            ibf_.clear(all_bins_);
            fill_ibf();
            refresh_agent();
            
        }

        void update_from_changes(std::vector<Change>& changes){


        } 

        void fill_ibf(){
            for (size_t i = 0; i < hists_.size(); ++i) {
                for (const auto& key :hists_[i].keys())
                    ibf_.emplace(key, seqan::hibf::bin_index{i});
            }
        }

        void precompute_distances(std::vector<label_t>& labels_cur){
            for (size_t i=0; i<data_.size(); i++ ){
                precomputed_distances_[i] = dist_.get(data_[i], labels_cur[i]);
            }
        }

    private:
        HistogramCollection& hists_;
        Distance& dist_;
        std::vector<dist_base_t> precomputed_distances_; 
        const std::vector<std::vector<token_t>>& data_;
        std::vector<size_t> indices_;
        std::vector<size_t> fallback_indices_;
        double fpr_ = 0.01;
        std::vector<seqan::hibf::bin_index> all_bins_;
        seqan::hibf::interleaved_bloom_filter ibf_;
        agent_type cluster_agent_;
        
        static auto make_ibf(
                HistogramCollection& hists,
                const std::vector<std::vector<token_t>>& data, size_t hash_fun_count=3, double fpr=0.01){
            size_t max_expected = histogram::estimate_maxhist_elems(hists); 
            return preselect::make_ibf(max_expected, hists.size(), hash_fun_count, fpr);
        }
    };

} // namespace ksets
