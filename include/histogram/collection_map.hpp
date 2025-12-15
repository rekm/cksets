#pragma once
#include <vector>
#include <string>
#include "map.hpp"
#include "collection_base.hpp"

namespace ksets::histogram {

template <typename HashPolicy, typename AllocatorPolicy = std::allocator<std::pair<const std::string,int>>>
class MapHistogramCollection
    : public BaseHistogramCollection<MapHistogramCollection<HashPolicy, AllocatorPolicy>>
{
    std::vector<MapHistogram<HashPolicy, AllocatorPolicy>> histograms_;
    size_t K_;
    HashPolicy hash_;
    AllocatorPolicy alloc_;

public:
    using MapHistType = ksets::histogram::MapHistogram<HashPolicy, AllocatorPolicy>;
    explicit MapHistogramCollection(size_t K, HashPolicy hash = {}, AllocatorPolicy alloc = {})
        : histograms_(K, MapHistogram<HashPolicy, AllocatorPolicy>(hash, alloc)), K_(K), alloc_(alloc) {}

    MapHistType& operator[](size_t i) { return histograms_[i]; }
    const MapHistType& operator[](size_t i) const { return histograms_[i]; }

    void increment(size_t cluster, const std::string& token) {
        histograms_[cluster].increment(token);
    }

    void decrement(size_t cluster, const std::string& token) {
        histograms_[cluster].decrement(token);
    }

    MapHistogramCollection clone_empty_impl() const {
        return MapHistogramCollection(K_, hash_, alloc_);
    }

    bool empty() const {
        bool res = false;
        for (auto const& h : histograms_) {
            res |= h.empty();
            if (res) break;
        }
        return res;
    }

    void clear(){
        for (auto& h : histograms_) {
            h.clear();
        }
    }

    int get(size_t cluster, const std::string& token) const {
        return histograms_[cluster].get(token);
    }

    void zero_out() {
        for (auto& h : histograms_) {
            h.zero_out();
        }
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

    std::vector<int> get_all(const std::string& token, CollectionRange<MapHistogramCollection>& range) const {
        std::vector<int> counts(histograms_.size(), 0);
        for (const auto cluster_idx : range) 
            counts[cluster_idx] = get(cluster_idx, token);
        return counts;
    }

    size_t size() const { return histograms_.size(); }
};

} // namespace ksets::histogram