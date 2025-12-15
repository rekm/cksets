#pragma once
#include <clustering/histogram_preselect_policy.hpp>
#include <histogram/collection_iterator.hpp>
#include <numeric>

namespace ksets {

    template <typename HistogramCollection>
    class HistPreselectNothing
        : public HistogramPreselectPolicy<HistPreselectNothing<HistogramCollection>, HistogramCollection>
    {
    public:
        using Base = HistogramPreselectPolicy<HistPreselectNothing<HistogramCollection>,
                            HistogramCollection>;
        using Range = histogram::CollectionRange<HistogramCollection>;
        using Base::Base;
        
        explicit HistPreselectNothing(HistogramCollection& hist,
                            const std::vector<std::vector<std::string>>& data)
            : Base(hist, data),  
            hists_(hist), data_(data), indices_(hist.size())
        {
            std::iota(indices_.begin(), indices_.end(), 0);
        }
        void before_iteration_impl(size_t it_idx) {}

        Range get_candidate_histograms_impl(const std::vector<std::string>& data_row) {
            return hists_.get_range(indices_);
        }


        void update_impl(std::vector<int>& labels_cur) {
        }

        

    private:
        HistogramCollection& hists_;
        std::vector<size_t> indices_;
        const std::vector<std::vector<std::string>>& data_;
    };

} // namespace ksets
