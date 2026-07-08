#pragma once
#include <ksets/types.hpp>
#include <vector>

namespace ksets {

struct ClusterResult
{
    std::vector<label_t> labels;
    size_t iterations;
    dist_base_t objective;
};


}//namespace ksets 