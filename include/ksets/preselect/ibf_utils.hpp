#pragma once 
#include <ksets/types.hpp>
#include <hibf/interleaved_bloom_filter.hpp> // for bin_index, interleaved_bloom_filter, bin_count, bin_size, hash_...
#include <hibf/misc/print.hpp>
#include <hibf/build/bin_size_in_bits.hpp>
#include <ksets/distance/jaccard.hpp>   
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
        std::vector<double> constant1_;
        std::vector<double> constant2_;
        std::vector<hist_count_t> hist_sums;
        HC &hists_;
        Distance &dist_;

        public:
            static constexpr bool enabled = true;

        JaccardThresholdPolicy(HC & hists,
                               Distance & dist) 
                : hists_(hists), dist_(dist) {
                    threshold_scratch_.resize(hists_.size());
                    constant1_.resize(hists_.size());
                    constant2_.resize(hists_.size());
                }

        void before_iteration(size_t j) {
            //Precompute constants for taylor approximation of threshold formula
            auto maximums = hists_.maximums(); 
            hist_sums = hists_.totals();
            for (size_t i=0; i<hists_.size(); i++){
                constant1_[i] = 1 / maximums[i];
                constant2_[i] = 1 / (maximums[i] * maximums[i]);
            }

        }

        std::vector<uint16_t> & compute_thresholds(double dist, size_t q_len) {
            for (size_t i=0; i<hists_.size(); i++){
                threshold_scratch_[i] = (hist_sums[i] + q_len) * dist * ( constant1_[i] + constant2_[i] * dist); 
            }
            return threshold_scratch_;
        }
    }; //Dynamic Jaccard Threshold ;

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

} // ksets::preselect