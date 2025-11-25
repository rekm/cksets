#pragma once
#include <vector>
#include <string>
#include "map_histogram.hpp"
#include "base_histogram_collection.hpp"

namespace ksets::histogram {

template <typename HashPolicy, typename AllocatorPolicy = std::allocator<std::pair<const std::string,int>>>
class MapHistogramCollection
    : public BaseHistogramCollection<MapHistogramCollection<HashPolicy, AllocatorPolicy>>
{
    std::vector<MapHistogram<HashPolicy, AllocatorPolicy>> histograms_;
public:
    using MapHistType = ksets::histogram::MapHistogram<HashPolicy, AllocatorPolicy>;
    explicit MapHistogramCollection(size_t K, HashPolicy hash = {}, AllocatorPolicy alloc = {})
        : histograms_(K, MapHistogram<HashPolicy, AllocatorPolicy>(hash, alloc)) {}

    MapHistType& operator[](size_t i) { return histograms_[i]; }
    const MapHistType& operator[](size_t i) const { return histograms_[i]; }

    void increment(size_t cluster, const std::string& token) {
        histograms_[cluster].increment(token);
    }

    int get(size_t cluster, const std::string& token) const {
        return histograms_[cluster].get(token);
    }

    std::vector<int> totals() const {
        std::vector<int> sums;
        sums.reserve(histograms_.size());
        for (const auto& h : histograms_) {
            sums.push_back(h.total()); // h.total() sums counts inside that histogram
        }
        return sums;
    }

    std::vector<int> get_all(const std::string& token) const {
        std::vector<int> counts;
        counts.reserve(histograms_.size());
        for (auto const& h : histograms_) counts.push_back(h.get(token));
        return counts;
    }

    size_t size() const { return histograms_.size(); }
};

} // namespace mylib::histogram