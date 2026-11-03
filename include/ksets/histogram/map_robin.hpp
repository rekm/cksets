#pragma once
#include <ksets/types.hpp>
#include <ksets/histogram/base.hpp>
#include <ksets/memory/allocator_policy.hpp>
#include <tsl/robin_map.h>
#include <iostream>
#include <vector>

namespace ksets {
namespace histogram {

template <
    class HashPolicy,
    class AllocatorPolicy = std::allocator<std::pair<const token_t,hist_count_t>>
>
class MapRobinHistogram : 
    public HistogramBase<MapRobinHistogram<HashPolicy, AllocatorPolicy >>
{
    using MapType = tsl::robin_map<
        token_t,
        hist_count_t,
        HashPolicy,
        std::equal_to<token_t>,
        AllocatorPolicy
    >;
    HashPolicy hash_;
    MapType freq;

public:
    MapRobinHistogram(HashPolicy hash, AllocatorPolicy alloc = {})
        : freq(0, hash, std::equal_to<token_t>(), alloc) , hash_(hash)
    {}

    bool increment(const token_t& token) {
        auto [it, inserted] = freq.try_emplace(token, 1);
        if (!inserted)
            ++it.value();
        return inserted; 
    }

    void zero_out(){
        for (auto it = freq.begin(); it != freq.end(); ++it) {
            it.value() = 0;
        }
    }

    bool empty() const {
        return freq.empty();
    }
    
    void clear(){
        freq.clear();
    }

    size_t size() const {
        return freq.size();
    }

    bool decrement(const token_t& token) {
        bool ret = false; 
        auto it = freq.find(token);
        if (it == freq.end()) {
            return ret; // nothing to undo
        }
        auto& value = it.value();
        if (--value == 0) {
            ret = true;
            freq.erase(it); // avoid zombie keys with value 0
        }
        return ret;
    }

    hist_count_t get(const token_t& token) const {
        auto it = freq.find(token);
        return it != freq.end() ? it->second : 0;
    }

    hist_count_t get_with_hash(const token_t& token, size_t h) const {
        auto it = freq.find(token, h);
        return it != freq.end() ? it->second : 0;
    }

    void set(const token_t& token, hist_count_t value) {
        freq[token] = value;
    }

    void reserve_impl(size_t s){
        freq.reserve(s);
    }

    void debug_print() const {
        for (const auto& kv : freq) {
            std::cout << kv.first << " -> " << kv.second << ", ";
        }
        std::cout << "\n";
    }

    hist_count_t maximum() const {
        auto it = std::max_element(
                freq.begin(), freq.end(),
                [](const auto& a, const auto& b) {
                    return a.second < b.second;
                }
            );
        return (it == freq.end()) ? hist_count_t{} : it->second;
    }

    hist_count_t minimum() const {
        auto it = std::min_element(
                freq.begin(), freq.end(),
                [](const auto& a, const auto& b) {
                    return a.second < b.second;
                }
            );
        return (it == freq.end()) ? hist_count_t{} : it->second;
    }

    hist_count_t total() const {
        hist_count_t sum = 0;
        for (const auto& kv : freq) {
            sum += kv.second;
        }
        return sum;
    }

    std::vector<hist_count_t> values(int max_hist_length = std::numeric_limits<int>::max()) const {
        std::vector<hist_count_t> out;
        out.reserve(std::min<size_t>(freq.size(), max_hist_length));

        size_t count = 0;
        for (auto const &kv : freq) {
            out.push_back(kv.second);
            if (++count == max_hist_length) break;
        }
        return out;
    }

    std::vector<token_t> keys(int max_hist_length = std::numeric_limits<int>::max()) const {
        std::vector<token_t> out;
        out.reserve(std::min<size_t>(freq.size(), max_hist_length));

        size_t count = 0;
        for (auto const &kv : freq) {
            out.push_back(kv.first);
            if (++count == max_hist_length) break;
        }
        return out;
    }
    
};


} // namespace histogram
} // namespace ksets