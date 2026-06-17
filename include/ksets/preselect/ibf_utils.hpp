#pragma once 
#include <ksets/types.hpp>
#include <hibf/interleaved_bloom_filter.hpp> // for bin_index, interleaved_bloom_filter, bin_count, bin_size, hash_...
#include <hibf/misc/print.hpp>
#include <hibf/build/bin_size_in_bits.hpp>
#include <ksets/distance/jaccard.hpp>


#include <ranges>
namespace ksets::preselect {

    // Forward Declarations 
    template<class HC, class Distance>
    class JaccardThresholdPolicy;


    template<class HC, class Distance>
    struct ThresholdPolicyFor; 

    // Implementations 

    template<typename Policy>
    struct threshold_policy_traits{
        static constexpr bool dynamic_threshold = false;
    };

    template<class HC, class Distance>
    struct threshold_policy_traits<
            JaccardThresholdPolicy<HC, Distance>
        >{
        static constexpr bool dynamic_threshold = true;
    };

    template<class HC, class Distance>
    class StaticLinearThresholdPolicy{
        public:
            static constexpr bool enabled = false;

            StaticLinearThresholdPolicy(HC const&, Distance const&) {}
            
            void before_iteration(size_t j) {}

            uint16_t compute_threshold(size_t q_len){
                return static_cast<uint16_t>(0.9999 + q_len * 0.5);
            }
    }; //Static Linear Threshold

    template<class HC>
    struct ThresholdPolicyFor<
            HC,
            ksets::distance::JaccardDistance<HC>>{
        using type =
            JaccardThresholdPolicy<
            HC,
            ksets::distance::JaccardDistance<HC>>;
    }; // Use Dynamic Jaccard For Jaccard Distances 

    template<class HC, class Distance>
    class JaccardThresholdPolicy{
        std::vector<uint16_t> threshold_scratch_;
        std::vector<double> min_constant1_;
        std::vector<double> min_constant2_;
        std::vector<double> max_constant1_;
        std::vector<double> max_constant2_;
        HC &hists_;
        Distance &dist_;

        public:
            static constexpr bool enabled = true;

        JaccardThresholdPolicy(HC & hists,
                               Distance & dist) 
                : hists_(hists), dist_(dist) {
                    threshold_scratch_.resize(hists_.size());
                    min_constant1_.resize(hists_.size());
                    min_constant2_.resize(hists_.size());
                    max_constant1_.resize(hists_.size());
                    max_constant2_.resize(hists_.size());
                }

        void before_iteration(size_t j) {
            //Precompute constants for taylor approximation of threshold formula
            auto const& maximums = hists_.maximums(); 
            auto const& minimums = hists_.minimums();
            auto const&hist_sums = hists_.totals();
            for (size_t i=0; i<hists_.size(); i++){
                double m = static_cast<double>(maximums[i]);
                double m2 = static_cast<double>(minimums[i]);
                min_constant1_[i] = 1.0 / m;
                min_constant2_[i] = -1.0 / (m * m);
                max_constant1_[i] = 1.0 / m2;
                max_constant2_[i] = -1.0 / (m2 * m2);
            }

        }

        std::vector<uint16_t> & compute_min_thresholds(double dist, size_t q_len) {
            auto const &hist_sums = hists_.totals();
            double alpha = 1.0 - dist;
            for (size_t i=0; i<hists_.size(); i++){
                if (hist_sums[i] == 0) {
                    threshold_scratch_[i] = std::numeric_limits<uint16_t>::max(); 
                } else {
                    threshold_scratch_[i] = (hist_sums[i] + q_len) * alpha * ( min_constant1_[i] + min_constant2_[i] * alpha); 
                }
            }
            return threshold_scratch_;
        }

        std::vector<uint16_t> & compute_max_thresholds(double dist, size_t q_len) {
            auto const &hist_sums = hists_.totals();
            double alpha = 1.0 - dist;
            for (size_t i=0; i<hists_.size(); i++){
                if (hist_sums[i] == 0) {
                    threshold_scratch_[i] = std::numeric_limits<uint16_t>::max();   
                } else {
                    threshold_scratch_[i] = (hist_sums[i] + q_len) * alpha * ( max_constant1_[i] + max_constant2_[i] * alpha);
                }
                 
            }
            return threshold_scratch_;
        }
    }; //Dynamic Jaccard Threshold ;

    

    static auto make_ibf(
                size_t max_elems, 
                size_t ibf_size, 
                size_t hash_fun_count=3, 
                double fpr=0.01){       
        seqan::hibf::build::bin_size_parameters params{};
        params.elements   = max_elems;
        params.hash_count = hash_fun_count;
        params.fpr        = fpr;

        return seqan::hibf::interleaved_bloom_filter{
            seqan::hibf::bin_count{ibf_size},
            seqan::hibf::bin_size{
                seqan::hibf::build::bin_size_in_bits(params)},
            seqan::hibf::hash_function_count{hash_fun_count}};
    } // make_ibf 

    template <std::ranges::forward_range value_range_t, typename IBFAgent>
    void membership_with_dynamic_min_threshold(value_range_t && values,
                                               std::vector<size_t> &out,
                                               std::vector<uint16_t> const &thresholds,
                                               IBFAgent &count_agent) {
        out.clear();
        auto & counts_per_bin = count_agent.bulk_count(std::forward<value_range_t>(values));
        assert(thresholds.size() == counts_per_bin.size()); 
                                             
        if (out.capacity() < counts_per_bin.size())
            seqan::hibf::unreachable();

        for (auto [bin, count] : seqan::stl::views::enumerate(counts_per_bin))
            if (count >= thresholds[bin])
                out.emplace_back(bin);
        
    }

    template <std::ranges::forward_range value_range_t, typename IBFAgent>
    void membership_with_static_min_threshold(value_range_t && values,
                                               std::vector<size_t> &out,
                                               uint16_t const threshold,
                                               IBFAgent &count_agent) {
        out.clear();
        auto & counts_per_bin = count_agent.bulk_count(std::forward<value_range_t>(values));
        assert(values.size() == counts_per_bin.size()); 
                                             
        if (out.capacity() < counts_per_bin.size())
            seqan::hibf::unreachable();

        for (auto [bin, count] : seqan::stl::views::enumerate(counts_per_bin))
            if (count >= threshold)
                out.emplace_back(bin);
        
    }

    template <std::ranges::forward_range value_range_t, typename IBFAgent>
    void membership_with_dynamic_max_threshold(value_range_t && values,
                                               std::vector<size_t> &out,
                                               std::vector<uint16_t> const &thresholds,
                                               IBFAgent &count_agent) {
        out.clear();
        auto & counts_per_bin = count_agent.bulk_count(std::forward<value_range_t>(values));
        assert(thresholds.size() == counts_per_bin.size()); 
                                             
        if (out.capacity() < counts_per_bin.size())
            seqan::hibf::unreachable();

        for (auto [bin, count] : seqan::stl::views::enumerate(counts_per_bin))
            if (count > thresholds[bin])
                out.emplace_back(bin);
        
    }

    template <std::ranges::forward_range value_range_t, typename IBFAgent>
    label_t estimate_best_cluster(value_range_t && values,
                               const std::vector<hist_count_t> &min_counts,
                               IBFAgent &count_agent) {
        size_t best_val = 0;
        label_t best_cluster = 0;
        auto & counts_per_bin = count_agent.bulk_count(std::forward<value_range_t>(values));
        assert(min_counts.size() == counts_per_bin.size()); 
        size_t curr_val = 0; 
        for (auto [bin, count] : seqan::stl::views::enumerate(counts_per_bin)){
            curr_val = count * min_counts[bin];
            if (curr_val > best_val){
                best_cluster = bin;
                best_val = curr_val;
            } 
        }
        return best_cluster;
    }


    enum class StaticThresholdMode {
            Linear,
            SqrtLow,
            SqrtHigh
    };

    inline uint16_t compute_static_threshold(StaticThresholdMode mode, size_t n) {
            switch (mode) {
                case StaticThresholdMode::Linear:
                    return static_cast<uint16_t>(0.9999 + n * 0.5);
                case StaticThresholdMode::SqrtLow:
                    return static_cast<uint16_t>(0.9999 + std::sqrt(n) * 0.1);
                case StaticThresholdMode::SqrtHigh:
                    return static_cast<uint16_t>(0.9999 + std::sqrt(n) * 1.0);
            }
            __builtin_unreachable();
    }
} // ksets::preselect