#pragma once
#include <ksets/types.hpp>
#include <ksets/histogram/base.hpp>
#include <ksets/memory/allocator_policy.hpp>
#include <iostream>

namespace ksets {
namespace histogram {

template <
    class HashPolicy,
    class AllocatorPolicy = std::allocator<std::pair<const token_t,int>>
>
class MapHistogram : 
    public HistogramBase<MapHistogram<HashPolicy, AllocatorPolicy >>
{
    using MapType = std::unordered_map<
        token_t,
        int,
        HashPolicy,
        std::equal_to<token_t>,
        AllocatorPolicy
    >;

    MapType freq;

public:
    MapHistogram(HashPolicy hash, AllocatorPolicy alloc = {})
        : freq(0, hash, std::equal_to<token_t>(), alloc)
    {}

    void increment(const token_t& token) {
        freq[token]++;
    }

    void zero_out(){
        for (auto& kv : freq) {
            kv.second = 0; 
        }
    }

    bool empty() const {
        return freq.empty();
    }
    
    void clear(){
        freq.clear();
    }

    void decrement(const token_t& token) {
        auto it = freq.find(token);
        if (it == freq.end()) {
            return; // nothing to undo
        }

        if (--it->second == 0) {
            freq.erase(it); // avoid zombie keys with value 0
        }
    }

    int get(const token_t& token) const {
        auto it = freq.find(token);
        return it != freq.end() ? it->second : 0;
    }

    void set(const token_t& token, int value) {
        freq[token] = value;
    }

    void debug_print() const {
        for (const auto& kv : freq) {
            std::cout << kv.first << " -> " << kv.second << ", ";
        }
        std::cout << "\n";
    }

    int total() const {
        int sum = 0;
        for (const auto& kv : freq) {
            sum += kv.second;
        }
        return sum;
    }

    std::vector<int> values(int max_hist_length = std::numeric_limits<int>::max()) const {
        std::vector<int> out;
        out.reserve(std::min<size_t>(freq.size(), max_hist_length));

        int count = 0;
        for (auto const &kv : freq) {
            out.push_back(kv.second);
            if (++count == max_hist_length) break;
        }
        return out;
    }

    std::vector<token_t> keys(int max_hist_length = std::numeric_limits<int>::max()) const {
        std::vector<token_t> out;
        out.reserve(std::min<size_t>(freq.size(), max_hist_length));

        int count = 0;
        for (auto const &kv : freq) {
            out.push_back(kv.first);
            if (++count == max_hist_length) break;
        }
        return out;
    }
    
};


} // namespace histogram
} // namespace ksets