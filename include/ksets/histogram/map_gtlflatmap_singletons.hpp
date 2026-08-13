#pragma once

#include <vector>
#include <utility>
#include <functional>
#include <algorithm>
#include <iostream>
#include <limits>

// Assuming gtl/phmap headers are available in your build environment
#include <gtl/phmap.hpp>
#include <gtl/phmap_utils.hpp>

namespace ksets::histogram {

template <
    class HashPolicy,
    class AllocatorPolicy = std::allocator<std::pair<const token_t, hist_count_t>>
>
class MapGtlFlatmapSingletonHistogram : 
    public HistogramBase<MapGtlFlatmapSingletonHistogram<HashPolicy, AllocatorPolicy>>
{
public:
    using MapType = gtl::flat_hash_map<
        token_t,
        hist_count_t,
        HashPolicy,
        std::equal_to<token_t>,
        AllocatorPolicy
    >;

    // Re-use allocator policy rebind for the singletons flat_hash_set
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
    SetType singletons; // Tracks tokens with an implicit count of 1

public:
    MapGtlFlatmapSingletonHistogram(HashPolicy hash, AllocatorPolicy alloc = {})
        : freq(0, hash, std::equal_to<token_t>(), alloc),
          singletons(0, hash, std::equal_to<token_t>(), SetAllocator(alloc)),
          hash_(hash)
    {}

    /**
     * @brief Increments the count for a given token.
     * @details If the token is unseen, it is placed in the singletons set (count = 1).
     *          If it was already a singleton, it gets promoted to the main freq map with a count of 2.
     *          If it is already in the main freq map, its count is incremented normally.
     * @return true if this is the first time the token has been seen in this histogram.
     */
    bool increment(const token_t& token) {
        // Case 1: Already present in the heavy frequency map
        auto it = freq.find(token);
        if (it != freq.end()) {
            ++(it->second);
            return false;
        }

        // Case 2: Present in the singletons set -> Promote to the map
        auto sit = singletons.find(token);
        if (sit != singletons.end()) {
            singletons.erase(sit);
            freq.emplace(token, static_cast<hist_count_t>(2));
            return false;
        }

        // Case 3: Brand new token -> Add to singletons set
        singletons.insert(token);
        return true; 
    }

    /**
     * @brief Decrements the count for a given token.
     * @details If the count drops to 1, the token is demoted from the map back to the singletons set.
     *          If it drops to 0 from the singletons set, it is completely removed.
     * @return true if the token was completely removed from the entire data structure.
     */
    bool decrement(const token_t& token) {
        // Check heavy frequency map first
        auto it = freq.find(token);
        if (it != freq.end()) {
            if (--(it->second) == 1) {
                // Demote back to singleton set to free the value memory slice
                freq.erase(it);
                singletons.insert(token);
            }
            return false;
        }

        // Check singletons set
        auto sit = singletons.find(token);
        if (sit != singletons.end()) {
            singletons.erase(sit);
            return true; // Avoids zombie keys and tracks complete removal
        }

        return false; // Nothing found to undo
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

    size_t size() const {
        return freq.size() + singletons.size();
    }

    hist_count_t get(const token_t& token) const {
        auto it = freq.find(token);
        if (it != freq.end()) {
            return it->second;
        }
        if (singletons.contains(token)) {
            return 1;
        }
        return 0;
    }

    hist_count_t get_with_hash(const token_t& token, size_t h) const {
        size_t raw = gtl::phmap_mix<sizeof(size_t)>()(h);
        
        auto it = freq.find(token, raw);
        if (it != freq.end()) {
            return it->second;
        }
        
        auto sit = singletons.find(token, raw);
        if (sit != singletons.end()) {
            return 1;
        }
        
        return 0;
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
        // Reserve half/proportional boundaries if cluster density characteristics are known
        freq.reserve(s / 2);
        singletons.reserve(s / 2);
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

    hist_count_t total() const {
        hist_count_t sum = static_cast<hist_count_t>(singletons.size());
        for (const auto& kv : freq) {
            sum += kv.second;
        }
        return sum;
    }

    hist_count_t maximum() const {
        if (freq.empty()) {
            return singletons.empty() ? hist_count_t{} : static_cast<hist_count_t>(1);
        }
        
        auto it = std::max_element(
            freq.begin(), freq.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            }
        );
        return it->second;
    }

    hist_count_t minimum() const {
        if (!singletons.empty()) {
            return static_cast<hist_count_t>(1);
        }
        if (freq.empty()) {
            return hist_count_t{};
        }
        
        auto it = std::min_element(
            freq.begin(), freq.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            }
        );
        return it->second;
    }

    const MapType& expose_map() const {
        return freq;
    }

    const SetType& expose_singletons() const {
        return singletons;
    }

    std::vector<hist_count_t> values(int max_hist_length = std::numeric_limits<int>::max()) const {
        std::vector<hist_count_t> out;
        size_t total_bound = freq.size() + singletons.size();
        out.reserve(std::min<size_t>(total_bound, max_hist_length));

        size_t count = 0;
        // First pull out the singletons
        for (size_t i = 0; i < singletons.size() && count < static_cast<size_t>(max_hist_length); ++i) {
            out.push_back(1);
            ++count;
        }
        // Then pull out the heavier keys
        for (auto const &kv : freq) {
            if (count == static_cast<size_t>(max_hist_length)) break;
            out.push_back(kv.second);
            ++count;
        }
        return out;
    }

    std::vector<token_t> keys(int max_hist_length = std::numeric_limits<int>::max()) const {
        std::vector<token_t> out;
        size_t total_bound = freq.size() + singletons.size();
        out.reserve(std::min<size_t>(total_bound, max_hist_length));

        size_t count = 0;
        for (auto const &k : singletons) {
            if (count == static_cast<size_t>(max_hist_length)) break;
            out.push_back(k);
            ++count;
        }
        for (auto const &kv : freq) {
            if (count == static_cast<size_t>(max_hist_length)) break;
            out.push_back(kv.first);
            ++count;
        }
        return out;
    }
};

} // namespace ksets::histogram