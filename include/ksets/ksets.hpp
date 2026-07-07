#pragma once 
#include <cstdint>
#include <vector>

namespace ksets {

enum class HistogramRebuild : uint8_t { 
    full, update
};

enum class PreselectStrategy : uint8_t { 
    none, ibf
};

struct ClusterOptions
{
    size_t clusters = 16;
    size_t max_hist_length = 0;
    size_t max_iterations = 50;
    uint64_t seed = 42;

    HistogramRebuild rebuild =
        HistogramRebuild::full;

    PreselectStrategy preselect =
        PreselectStrategy::ibf;
};


struct ClusterResult
{
    std::vector<size_t> labels;
    size_t iterations;
    double objective;
};

}