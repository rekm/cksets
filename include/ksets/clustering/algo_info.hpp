#pragma once 

#include <ksets/telemetry/base.hpp>
#include <numeric>

namespace ksets {
    enum class EndReason {
        DistanceSumStagnated,
        DistanceIncreased,
        IterationLimitReached,
        Interupt,
    };
 
    
    struct AlgorithmStart {
        static constexpr telemetry::TelemetryLevel level =
            telemetry::TelemetryLevel::basic;

        size_t n_items;
        size_t k;
        size_t max_iter;
        bool histograms_provided;
    };

    // Telemetry Events that can bew interceptet by a suitable Sink
    // They are compiled out, when a NullSink is used.

    struct HistogramStats {
        size_t min_len;
        size_t max_len;
        double avg_len;                     // average histogram length
        size_t median_len;                  // median histogram length
        double sum_distances; // total distance sum
        std::vector<hist_count_t> min_vals;
        std::vector<hist_count_t> max_vals;         
        std::vector<size_t> lengths;    // raw histogram lengths
        std::vector<size_t> label_sortorder;
        std::vector<double> deciles;
    };

    struct IterationMetrics {
        static constexpr telemetry::TelemetryLevel level =
            telemetry::TelemetryLevel::basic;

        size_t iter;
        double objective;
        double delta;
        std::chrono::high_resolution_clock::time_point timestamp;
    };

    struct RunSummary {
        static constexpr telemetry::TelemetryLevel level =
            telemetry::TelemetryLevel::basic;
        static constexpr telemetry::TelemetryCategory category =
            telemetry::TelemetryCategory::summary;

        size_t iterations;
        double sum_distances;
        enum EndReason reason; 
        HistogramStats hist;
    };

    struct PreselectSavingsTelemetry{
        static constexpr telemetry::TelemetryLevel level =
            telemetry::TelemetryLevel::debug;
        uint64_t total_tokens;
        uint64_t K; 
        uint64_t total_hists_touched;
    };

    struct DistanceMeasureTelemetry{
        static constexpr telemetry::TelemetryLevel level=
            telemetry::TelemetryLevel::trace;
        
        size_t iter;
        size_t data_row;
        std::vector<double> distances;
    };

    struct LabelSelectTelemetry{
        static constexpr telemetry::TelemetryLevel level=
            telemetry::TelemetryLevel::trace;
        size_t iter;
        size_t data_row;
        label_t old_label;
        label_t new_label;
        double old_dist;
        double new_dist;
    };

    inline HistogramStats compute_hist_stats(const auto& histograms) {
        HistogramStats stats;
        stats.lengths.reserve(histograms.size());
        stats.max_vals = histograms.maximums();
        stats.min_vals = histograms.minimums();
        double sum_len = 0;
        size_t num_hists = histograms.size();
        for (size_t idx=0; idx < num_hists; ++idx) {
            stats.lengths.push_back(histograms[idx].size());
            sum_len += histograms[idx].size();
        }
        stats.avg_len = sum_len / histograms.size();
        // The following is a over-engineered solution to an inplace sort of 
        // multiple vectors and highly cumbersome and pointless in this place.

        // Gemini botched the first attempt to implement the algorithm.
        // Generate an index array
        std::vector<size_t> indices(num_hists);
        std::iota(indices.begin(), indices.end(), 0);

        // Sort the indices based on the values residing inside stats.lengths
        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            return stats.lengths[a] < stats.lengths[b];
        });
        stats.label_sortorder = indices;
        // Gemini left out the inverse lookup and used the reordered indices directly.
        // Which is certainly a choice and very human at that.   
        std::vector<size_t> dest(num_hists);
        for (size_t i = 0; i < num_hists; ++i) {
            dest[indices[i]] = i;
        }
        // Reorder all three vectors in-place using the sorted indices to avoid allocations
        // This permutation algorithm guarantees O(N) time complexity and O(1) extra space.
        // Also Gemini not wanting to name sources 
        for (size_t i = 0; i < num_hists; ++i) {
            while (dest[i] != i) {
                size_t target_idx = dest[i];
                
                // Swap elements across all parallel data structures
                std::swap(stats.lengths[i], stats.lengths[target_idx]);
                std::swap(stats.max_vals[i], stats.max_vals[target_idx]);
                std::swap(stats.min_vals[i], stats.min_vals[target_idx]);
                
                // Fix the index map to track the updated position
                std::swap(dest[i], dest[target_idx]);
            }
        }
        
        stats.min_len = stats.lengths.front();
        stats.max_len = stats.lengths.back();
        size_t n = stats.lengths.size();
        stats.median_len = (n % 2 == 0) ? (stats.lengths[n/2 - 1] + stats.lengths[n/2]) / 2
                                    : stats.lengths[n/2];

        size_t chunk_size = std::max(n / 10, size_t{1});
        stats.deciles.reserve(10);
        for (auto chunk : stats.lengths | std::views::chunk(chunk_size)) {
            double chunk_avg = std::reduce(chunk.begin(), chunk.end()) / double(chunk.size());
            stats.deciles.push_back(chunk_avg);
        }

        return stats;
    }
    
}