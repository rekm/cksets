#pragma once
#include <ksets/clustering/histogram_preselect_policy.hpp>
#include <ksets/histogram/collection_iterator.hpp>
#include <ksets/histogram/utils.hpp>
#include <ksets/preselect/ibf_utils.hpp>
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

    

    template <typename HistogramCollection, typename Distance>
    class HistPreselectIBF
        : public HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection, Distance>, HistogramCollection>
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
                seqan::hibf::interleaved_bloom_filter::counting_agent_type,
                seqan::hibf::interleaved_bloom_filter::membership_agent_type>;    
    public:
        using Base = HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection, Distance>,
                            HistogramCollection>;
        using Base::Base;

        
        
        explicit HistPreselectIBF(HistogramCollection& hist,
                            const std::vector<std::vector<token_t>>& data, Distance& dist, double fpr=0.005)
            : Base(hist, data),  
            hists_(hist), data_(data), 
            threshold_policy_(hist, dist),
            indices_(hist.size()), dist_(dist),
            fpr_(fpr), 
            ibf_(make_ibf(hist, data, 3u, fpr))
        {
            std::iota(indices_.begin(), indices_.end(), 0);
            std::iota(fallback_indices_.begin(), fallback_indices_.end(), 0);
            all_bins_.reserve(ibf_.bin_count());
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

        void before_iteration_impl(size_t it_idx) {
            threshold_policy_.before_iteration(it_idx);
            if(it_idx == 0){
                for (size_t i = 0; i < hists_.size(); ++i) {
                    for (const auto& key :hists_[i].keys())
                        ibf_.emplace(key, seqan::hibf::bin_index{i});
                }
                refresh_agent();
            }
        }

        auto get_candidate_histograms_impl(const std::vector<token_t>& data_row) {
            //uint16_t threshold =
            //    static_cast<uint16_t>(data_row.size() * fpr_ + 0.999999);
            if constexpr (dynamic_threshold_)
            {
                auto const& thresholds = threshold_policy_.compute_thresholds(dist, data_row.size());
                // Code using ibf_.counting_agent() and storing the index results in res buffer 
            }
            else
            {
                uint16_t threshold =  threshold_policy_.compute_threshold(data_row.size());
                auto const& res = cluster_agent_.membership_for(data_row, threshold);
            }
            
            
            //indices_.clear();
            //indices_.insert(indices_.end(), res.begin(), res.end());
            if (res.empty()){
                indices_.assign(fallback_indices_.begin(), fallback_indices_.end());
            } else {
                indices_.assign(res.begin(), res.end());
                //indices_ = cluster_agent_.membership_for(data_row, threshold);
            }
            
            return hists_.get_range(indices_);
            //return hists_.get_range(res);
        }


        void update_impl(std::vector<label_t>& labels_cur) {
            ibf_.clear(all_bins_);
            before_iteration_impl(0);
            
        }

        void update_from_changes(std::vector<Change>& changes){


        } 


    private:
        HistogramCollection& hists_;
        Distance& dist_;
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
