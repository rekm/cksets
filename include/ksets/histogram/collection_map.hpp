#pragma once
#include <vector>
#include <string>
#include <ksets/types.hpp>
#include <ksets/histogram/map.hpp>
#include <ksets/histogram/map_gtlflatmap.hpp>
#include <ksets/histogram/collection_base.hpp>

namespace ksets::histogram {

using MapVal = hist_count_t;


template < 
    typename HashPolicy,
    template<typename, typename> typename MapTemplate = ksets::histogram::MapGtlFlatmapHistogram,
    typename AllocatorPolicy = std::allocator<std::pair<const token_t,label_t>>>
class MapHistogramCollection
    : public BaseHistogramCollection<MapHistogramCollection< HashPolicy, MapTemplate, AllocatorPolicy>>
{
    using MapHistType = MapTemplate<HashPolicy, AllocatorPolicy>;
    //using MapHistType = ksets::histogram::MapRobinHistogram<HashPolicy, AllocatorPolicy>;
    std::vector<MapHistType> histograms_;
    size_t K_;
    HashPolicy hash_;
    AllocatorPolicy alloc_;
    mutable std::vector<MapVal> totals_; 
    mutable std::vector<MapVal> mins_;
    mutable std::vector<MapVal> maxs_;
    //cache_flags 
    mutable bool dirty_totals_ = true;
    mutable bool dirty_mins_ = true;
    mutable bool dirty_maxs_ = true;
    //mutable std::vector<size_t> row_hashes_;

public:
    
    explicit MapHistogramCollection(size_t K, HashPolicy hash = {}, AllocatorPolicy alloc = {})
        : histograms_(K, MapHistType(hash, alloc)), K_(K), alloc_(alloc), hash_(hash), totals_(K),
          mins_(K), maxs_(K) {}

    MapHistType& operator[](size_t i) { return histograms_[i]; }
    const MapHistType& operator[](size_t i) const { return histograms_[i]; }

    bool increment_impl(size_t cluster, const token_t& token) {
        invalidate_cache();
        return histograms_[cluster].increment(token);
    }

    bool decrement_impl(size_t cluster, const token_t& token) {
        invalidate_cache();
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
        invalidate_cache();
    }

    MapVal get_impl(size_t cluster, const token_t& token) const {
        return histograms_[cluster].get(token);
    }

    void set_impl(size_t cluster, const token_t& token, MapVal val){
        histograms_[cluster].set(token, val);
    }

    void zero_out() {
        for (auto& h : histograms_) {
            h.zero_out();
        }
        invalidate_cache();
    }


    const std::vector<MapVal> & totals() const {
        if (dirty_totals_){ 
            for (size_t i=0; i < K_; i++) 
                totals_[i] = histograms_[i].total();
            dirty_totals_ = false; 
        }
        return totals_;
    }

    const std::vector<MapVal> & maximums() const {
        if (dirty_maxs_){ 
            for (size_t i=0; i < K_; i++) 
                maxs_[i] = histograms_[i].maximum();
            dirty_maxs_ = false; 
        }
        return maxs_;
    }

    const std::vector<MapVal> & minimums() const {
        if (dirty_mins_){ 
            for (size_t i=0; i < K_; i++) 
                mins_[i] = histograms_[i].minimum();
            dirty_mins_ = false;
        }
        return mins_;
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

    void invalidate_cache(){
        dirty_totals_ = true;
        dirty_mins_ = true;
        dirty_maxs_ = true;
    }
};

} // namespace ksets::histogram