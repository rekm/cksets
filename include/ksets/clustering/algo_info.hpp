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
        double avg;                     // average histogram length
        size_t median;                  // median histogram length
        double sum_distances;           // total distance sum
        std::vector<size_t> lengths;    // raw histogram lengths
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
        double sum_len = 0;
        size_t num_hists = histograms.size();
        for (size_t idx=0; idx < num_hists; ++idx) {
            stats.lengths.push_back(histograms[idx].size());
            sum_len += histograms[idx].size();
        }
        stats.avg = sum_len / histograms.size();

        std::sort(stats.lengths.begin(), stats.lengths.end());
        size_t n = stats.lengths.size();
        stats.median = (n % 2 == 0) ? (stats.lengths[n/2 - 1] + stats.lengths[n/2]) / 2
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