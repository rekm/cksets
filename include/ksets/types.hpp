#pragma once
#include <cstdint>

namespace ksets {
    using token_t = std::uint64_t;
    using label_t = size_t;
    using cluster_t = size_t;
    using hist_count_t = size_t;
    using dist_base_t = double;

    struct Change {
        size_t row_idx;
        size_t old_cluster;
        size_t new_cluster;
        bool old_token_removed = false;
        bool new_token_created = false;
    };

}