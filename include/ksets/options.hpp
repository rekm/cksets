#pragma once

namespace ksets {

enum class HistogramRebuild : uint8_t { 
    full, update
};

enum class PreselectStrategy : uint8_t { 
    none, ibf
};

enum class DistanceType : uint8_t {
    jaccard, cosine
};

enum class HashStrategy : uint8_t {
    murmur, prehashed
};


struct ClusterOptions
{
    size_t clusters = 16;
    size_t max_hist_length = 0;
    size_t max_iterations = 50;
    uint64_t seed = 42;

    HashStrategy      hash      = HashStrategy::murmur;
    DistanceType      distance  = DistanceType::jaccard;
    HistogramRebuild  rebuild   = HistogramRebuild::full;
    PreselectStrategy preselect = PreselectStrategy::ibf;
};

} // namespace ksets 