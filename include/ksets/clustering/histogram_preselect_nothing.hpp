#pragma once
#include <ksets/clustering/histogram_preselect_policy.hpp>
#include <ksets/histogram/collection_iterator.hpp>
#include <ksets/types.hpp>
#include <numeric>

namespace ksets {

    template <typename HistogramCollection, typename Distance>
    class HistPreselectNothing
        : public HistogramPreselectPolicy<HistPreselectNothing<HistogramCollection, Distance>, HistogramCollection, Distance>
    {
    public:
        using Base = HistogramPreselectPolicy<HistPreselectNothing<HistogramCollection, Distance>,
                            HistogramCollection, Distance>;
        using Base::Base;
        
        explicit HistPreselectNothing(HistogramCollection& hist,
                            const std::vector<std::vector<token_t>>& data, Distance &dist)
            : Base(hist, data, dist),  
            hists_(hist), data_(data), indices_(hist.size())
        {
            std::iota(indices_.begin(), indices_.end(), 0);
        }
        void before_iteration_impl(size_t it_idx, std::vector<label_t>& labels_cur) {}

        auto get_candidate_histograms_impl(const std::vector<token_t>& data_row, size_t row_idx) {
            return hists_.get_range(indices_);
        }


        void update_impl(std::vector<label_t>& labels_cur) {
        }

        

    private:
        HistogramCollection& hists_;
        std::vector<size_t> indices_;
        const std::vector<std::vector<token_t>>& data_;
    };

} // namespace ksets
