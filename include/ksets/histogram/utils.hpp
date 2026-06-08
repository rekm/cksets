#pragma once
#include <algorithm>
#include <cmath>


namespace ksets::histogram {

    template <typename HistogramCollection>
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
} // ksets::histogram