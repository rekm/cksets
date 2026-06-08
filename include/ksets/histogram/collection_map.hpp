#pragma once
#include <vector>
#include <string>
#include <ksets/types.hpp>
#include <ksets/histogram/map.hpp>
#include <ksets/histogram/map_gtlflatmap.hpp>
#include <ksets/histogram/map_robin.hpp>
#include <ksets/histogram/collection_base.hpp>

namespace ksets::histogram {

using MapVal = hist_count_t;


template <typename HashPolicy, typename AllocatorPolicy = std::allocator<std::pair<const token_t,label_t>>>
class MapHistogramCollection
    : public BaseHistogramCollection<MapHistogramCollection<HashPolicy, AllocatorPolicy>>
{
    using MapHistType = ksets::histogram::MapGtlFlatmapHistogram<HashPolicy, AllocatorPolicy>;
    //using MapHistType = ksets::histogram::MapRobinHistogram<HashPolicy, AllocatorPolicy>;
    std::vector<MapHistType> histograms_;
    size_t K_;
    HashPolicy hash_;
    AllocatorPolicy alloc_;
    //mutable std::vector<size_t> row_hashes_;

public:
    
    explicit MapHistogramCollection(size_t K, HashPolicy hash = {}, AllocatorPolicy alloc = {})
        : histograms_(K, MapHistType(hash, alloc)), K_(K), alloc_(alloc), hash_(hash) {}

    MapHistType& operator[](size_t i) { return histograms_[i]; }
    const MapHistType& operator[](size_t i) const { return histograms_[i]; }

    bool increment(size_t cluster, const token_t& token) {
        return histograms_[cluster].increment(token);
    }

    bool decrement(size_t cluster, const token_t& token) {
        return histograms_[cluster].decrement(token);
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

    MapVal get(size_t cluster, const token_t& token) const {
        return histograms_[cluster].get(token);
    }

    void zero_out() {
        for (auto& h : histograms_) {
            h.zero_out();
        }
    }

    std::vector<MapVal> totals() const {
        std::vector<MapVal> sums;
        sums.reserve(histograms_.size());
        for (const auto& h : histograms_) {
            sums.push_back(h.total()); // h.total() sums counts inside that histogram
        }
        return sums;
    }

    std::vector<MapVal> maximums() const {
        std::vector<MapVal> sums;
        sums.reserve(histograms_.size());
        for (const auto& h : histograms_) {
            sums.push_back(h.maximum()); // h.total() sums counts inside that histogram
        }
        return sums;
    }

    void get_all(const token_t& token, std::vector<MapVal>& out) const {
        for (auto const& h : histograms_) out.push_back(h.get(token));
    }

    template <class Alloc, class IndexContainer>
    void get_all(const token_t& token, std::vector<MapVal, Alloc>& out, CollectionRange<MapHistogramCollection, IndexContainer> range) const {
        size_t h  = hash_.hash(token);
        for (const auto cluster_idx : range) 
            out[cluster_idx] = histograms_[cluster_idx].get_with_hash(token, h);
    }

    template <typename CountType, class Alloc, class IndexContainer>
    void get_all(
            const std::vector<token_t>& token_row, 
            std::vector<CountType, Alloc>& out_sum,
            std::vector<CountType, Alloc>& out_misses, 
            CollectionRange<MapHistogramCollection, IndexContainer> range) const {

        //if (row_hashes_.size() < token_row.size())
        //    row_hashes_.resize(token_row.size());
        //row_hashes_.resize(token_row.size());
        std::vector<size_t> row_hashes(token_row.size());
        for (size_t i=0; i<token_row.size(); ++i) 
            row_hashes[i] = hash_.hash(token_row[i]);
        for (const cluster_t cluster_idx : range){
            auto &cur_hist = histograms_[cluster_idx]; 
            CountType misses = 0;
            CountType total = 0;
            for (size_t i=0; i<token_row.size(); ++i) {
                //auto v = cur_hist.get(token_row[i]);
                auto v = cur_hist.get_with_hash(token_row[i], row_hashes[i]);
                total += v;
                misses += (v == 0);
            }
            out_misses[cluster_idx] = misses; 
            out_sum[cluster_idx] = total;
        }
    }

    size_t size() const { return histograms_.size(); }

    
};

} // namespace ksets::histogram