#pragma once
#include <cstdint>

namespace ksets {
    using token_t = std::uint64_t;
    using label_t = std::uint16_t;
    using cluster_t = std::uint16_t;
    using hist_count_t = std::uint32_t;
    using dist_base_t = double;

    struct Change {
        size_t row_idx;
        cluster_t old_cluster;
        cluster_t new_cluster;
        bool old_token_removed = false;
        bool new_token_created = false;
    };

}