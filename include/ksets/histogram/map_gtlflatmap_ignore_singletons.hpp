#pragma once
#include <vector>
#include <utility>
#include <functional>
#include <algorithm>
#include <iostream>
#include <limits>

#include <gtl/phmap.hpp>
#include <gtl/phmap_utils.hpp>

namespace ksets::histogram {
template <
    class HashPolicy,
    class AllocatorPolicy = std::allocator<std::pair<const token_t, hist_count_t>>
>
class MapGtlFlatmapIgnoreSingletonHistogram : 
    public HistogramBase<MapGtlFlatmapIgnoreSingletonHistogram<HashPolicy, AllocatorPolicy>>{

public:

    using MapType = gtl::flat_hash_map<
        token_t,
        hist_count_t,
        HashPolicy,
        std::equal_to<token_t>,
        AllocatorPolicy
    >;

    using SetAllocator = typename std::allocator_traits<AllocatorPolicy>::template rebind_alloc<token_t>;
    using SetType = gtl::flat_hash_set<
        token_t,
        HashPolicy,
        std::equal_to<token_t>,
        SetAllocator
    >;
private:
    HashPolicy hash_;
    MapType freq;

    SetType singletons; // Always tracks true singletons internally
public:
    MapGtlFlatmapIgnoreSingletonHistogram(HashPolicy hash, AllocatorPolicy alloc = {})
        : freq(0, hash, std::equal_to<token_t>(), alloc),
          singletons(0, hash, std::equal_to<token_t>(), SetAllocator(alloc)),
          hash_(hash)
    {}

    bool increment(const token_t& token) {
        // Case 1: Token is already present in the heavy frequency map
        auto it = freq.find(token);
        if (it != freq.end()) {
            ++(it->second);
            return false;
        }

        // Case 2: Token is currently a singleton -> Promote to the map with a count of 2
        auto sit = singletons.find(token);

        if (sit != singletons.end()) {
            singletons.erase(sit);
            freq.emplace(token, static_cast<hist_count_t>(2));
            return false;
        }

        // Case 3: Brand new token -> Add to tracking set
        singletons.insert(token);
        return true; 
    }

    bool decrement(const token_t& token) {
        auto it = freq.find(token);
        if (it != freq.end()) {
            if (--(it->second) == 1) {
                freq.erase(it);
                singletons.insert(token);
            }
            return false;

        }

        auto sit = singletons.find(token);
        if (sit != singletons.end()) {
            singletons.erase(sit);
            return true;
        }

        return false;
    }

    void zero_out() {
        singletons.clear();
        for (auto it = freq.begin(); it != freq.end(); ++it) {
            it->second = 0;
        }
    }

    bool empty() const {

        return freq.empty() && singletons.empty();
    }
    
    void clear() {
        freq.clear();
        singletons.clear();
    }

    // Adaptive Sizing reporting
    size_t size() const {
        return freq.empty() ? singletons.size() : freq.size();
    }

    // Adaptive Lookup reporting
    hist_count_t get(const token_t& token) const {
        if (!freq.empty()) {
            auto it = freq.find(token);
            return it != freq.end() ? it->second : 0;
        }

        // Fallback: If freq is empty (e.g. immediately after seeding), expose singletons as count 1
        return singletons.contains(token) ? 1 : 0;
    }

    // Adaptive Lookup with precalculated hash
    hist_count_t get_with_hash(const token_t& token, size_t h) const {
        size_t raw = gtl::phmap_mix<sizeof(size_t)>()(h);
        if (!freq.empty()) {
            auto it = freq.find(token, raw);
            return it != freq.end() ? it->second : 0;
        }
        auto sit = singletons.find(token, raw);
        return sit != singletons.end() ? 1 : 0;
    }

    void set(const token_t& token, hist_count_t value) {
        if (value == 1) {
            freq.erase(token);
            singletons.insert(token);

        } else if (value > 1) {
            singletons.erase(token);
            freq[token] = value;
        } else {
            freq.erase(token);
            singletons.erase(token);
        }
    }

    void reserve_impl(size_t s) {
        freq.reserve(s / 2);
        singletons.reserve(s / 2);
    }

    hist_count_t total() const {
        if (!freq.empty()) {
            hist_count_t sum = 0;
            for (const auto& kv : freq) {
                sum += kv.second;

            }
            return sum;
        }
        return static_cast<hist_count_t>(singletons.size());
    }

    hist_count_t maximum() const {
        if (!freq.empty()) {
            auto it = std::max_element(
                freq.begin(), freq.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
            );
            return it->second;
        }
        return singletons.empty() ? hist_count_t{} : static_cast<hist_count_t>(1);
    }

    hist_count_t minimum() const {
        if (!freq.empty()) {

            auto it = std::min_element(
                freq.begin(), freq.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
            );
            return it->second;
        }
        return singletons.empty() ? hist_count_t{} : static_cast<hist_count_t>(1);
    }

    const MapType& expose_map() const {
        return freq;
    }

    const SetType& expose_singletons() const {
        return singletons;
    }

    std::vector<hist_count_t> values(int max_hist_length = std::numeric_limits<int>::max()) const {
        std::vector<hist_count_t> out;

        if (!freq.empty()) {
            out.reserve(std::min<size_t>(freq.size(), max_hist_length));
            size_t count = 0;
            for (auto const &kv : freq) {
                if (count == static_cast<size_t>(max_hist_length)) break;
                out.push_back(kv.second);
                ++count;
            }
        } else {
            out.reserve(std::min<size_t>(singletons.size(), max_hist_length));
            size_t count = 0;
            for (size_t i = 0; i < singletons.size() && count < static_cast<size_t>(max_hist_length); ++i) {
                out.push_back(1);
                ++count;
            }
        }
        return out;
    }

    void debug_print() const {
        std::cout << "[Singletons]: ";
        for (const auto& k : singletons) {
            std::cout << k << " -> 1, ";
        }
        std::cout << "\n[Map]: ";
        for (const auto& kv : freq) {
            std::cout << kv.first << " -> " << kv.second << ", ";
        }
        std::cout << "\n";
    }
    
    std::vector<token_t> keys(int max_hist_length = std::numeric_limits<int>::max()) const {
        std::vector<token_t> out;
        if (!freq.empty()) {
            out.reserve(std::min<size_t>(freq.size(), max_hist_length));
            size_t count = 0;
            for (auto const &kv : freq) {
                if (count == static_cast<size_t>(max_hist_length)) break;
                out.push_back(kv.first);
                ++count;
            }
        } else {
            out.reserve(std::min<size_t>(singletons.size(), max_hist_length));
            size_t count = 0;
            for (auto const &k : singletons) {
                if (count == static_cast<size_t>(max_hist_length)) break;
                out.push_back(k);
                ++count;
            }
        }

        return out;
    }
};

} // namespace ksets::histogram
