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
    class HistPreselectIBF
        : public HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection>, HistogramCollection>
    {
    public:
        using Base = HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection>,
                            HistogramCollection>;
        using Base::Base;
        
        explicit HistPreselectIBF(HistogramCollection& hist,
                            const std::vector<std::vector<token_t>>& data, double fpr=0.005)
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
                cluster_agent_ = ibf_.membership_agent();
            }
        }

        auto get_candidate_histograms_impl(const std::vector<token_t>& data_row) {
            //uint16_t threshold =
            //    static_cast<uint16_t>(data_row.size() * fpr_ + 0.999999);
            uint16_t threshold =  static_cast<uint16_t>(0.9999 + std::sqrt(data_row.size()) * 1.2);
            if (threshold < 2)
                threshold = 2;

            //return hists_.get_range(cluster_agent_.membership_for(data_row, threshold));
            auto const& res = cluster_agent_.membership_for(data_row, threshold);
            //indices_.clear();
            //indices_.insert(indices_.end(), res.begin(), res.end());
            indices_.assign(res.begin(), res.end());
            //indices_ = cluster_agent_.membership_for(data_row, threshold);
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
        const std::vector<std::vector<token_t>>& data_;
        std::vector<size_t> indices_;
        double fpr_ = 0.01;
        std::vector<seqan::hibf::bin_index> all_bins_;
        seqan::hibf::interleaved_bloom_filter ibf_;
        seqan::hibf::interleaved_bloom_filter::membership_agent_type cluster_agent_;
        
        

        static auto make_ibf(
                HistogramCollection& hists,
                const std::vector<std::vector<token_t>>& data, size_t hash_fun_count=3, double fpr=0.01){
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

        seqan::hibf::build::bin_size_parameters params{};
        params.elements   = 4 * std::max(max_expected, max_elements);
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
