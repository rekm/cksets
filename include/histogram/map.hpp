#pragma once
#include <histogram/base.hpp>
#include <memory/allocator_policy.hpp>
#include <iostream>

namespace ksets {
namespace histogram {

template <
    class HashPolicy,
    class AllocatorPolicy = std::allocator<std::pair<const std::string,int>>
>
class MapHistogram : 
    public HistogramBase<MapHistogram<HashPolicy, AllocatorPolicy >>
{
    using MapType = std::unordered_map<
        std::string,
        int,
        HashPolicy,
        std::equal_to<std::string>,
        AllocatorPolicy
    >;

    MapType freq;

public:
    MapHistogram(HashPolicy hash, AllocatorPolicy alloc = {})
        : freq(0, hash, std::equal_to<std::string>(), alloc)
    {}

    void increment(const std::string& token) {
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

    void decrement(const std::string& token) {
        auto it = freq.find(token);
        if (it == freq.end()) {
            return; // nothing to undo
        }

        if (--it->second == 0) {
            freq.erase(it); // avoid zombie keys with value 0
        }
    }

    int get(const std::string& token) const {
        auto it = freq.find(token);
        return it != freq.end() ? it->second : 0;
    }

    void set(const std::string& token, int value) {
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
    
};


} // namespace histogram
} // namespace ksets