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


namespace ksets {

    

    template <typename HistogramCollection>
    class HistPreselectIBF
        : public HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection>, HistogramCollection>
    {
    public:
        using Base = HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection>,
                            HistogramCollection>;
        using Range = histogram::CollectionRange<HistogramCollection>;
        using Base::Base;
        
        explicit HistPreselectIBF(HistogramCollection& hist,
                            const std::vector<std::vector<token_t>>& data, double fpr=0.01)
            : Base(hist, data),  
            hists_(hist), data_(data), indices_(hist.size()),
            fpr_(fpr), 
            ibf_(make_ibf(hist, data, 3u, fpr))
        {
            std::iota(indices_.begin(), indices_.end(), 0);
            all_bins_.reserve(ibf_.bin_count());
            for (size_t i = 0; i < ibf_.bin_count(); ++i)
                all_bins_.push_back(seqan::hibf::bin_index{i});
            cluster_agent_ = ibf_.membership_agent();
        }
        void before_iteration_impl(size_t it_idx) {
            if(it_idx == 0){
                for (size_t i = 0; i < hists_.size(); ++i) {
                    for (const auto& key :hists_[i].keys())
                        ibf_.emplace(key, seqan::hibf::bin_index{i});
                }
            }
        }

        Range get_candidate_histograms_impl(const std::vector<token_t>& data_row) {
            return hists_.get_range(cluster_agent_.membership_for(data_row, std::max( uint16_t{data_row.size() * fpr_},uint16_t{1})));
        }


        void update_impl(std::vector<label_t>& labels_cur) {
            ibf_.clear(all_bins_);
            before_iteration_impl(0);
            cluster_agent_ = ibf_.membership_agent();
        }

        void update_from_changes(std::vector<Change>& changes){


        } 


    private:
        double fpr_ = 0.01;
        std::vector<seqan::hibf::bin_index> all_bins_;
        seqan::hibf::interleaved_bloom_filter ibf_;
        seqan::hibf::interleaved_bloom_filter::membership_agent_type cluster_agent_;
        HistogramCollection& hists_;
        std::vector<size_t> indices_;
        const std::vector<std::vector<token_t>>& data_;

        static auto make_ibf(
                HistogramCollection& hists,
                const std::vector<std::vector<token_t>>& data, size_t hash_fun_count=3, double fpr=0.01){
        size_t avg_elements = 1;
        for (size_t idx=0; idx< hists.size(); idx++)
            avg_elements = std::max<size_t>(avg_elements, hists[idx].size());

        seqan::hibf::build::bin_size_parameters params{};
        params.elements   = 2 * avg_elements;
        params.hash_count = hash_fun_count;
        params.fpr        = fpr;

        return seqan::hibf::interleaved_bloom_filter{
            seqan::hibf::bin_count{hists.size()},
            seqan::hibf::bin_size{
                seqan::hibf::build::bin_size_in_bits(params)},
            seqan::hibf::hash_function_count{hash_fun_count}};
        }
    };

} // namespace ksets
