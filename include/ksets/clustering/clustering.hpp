
#pragma once
#include <ksets/histogram/collection_base.hpp>
#include <ksets/clustering/histogram_update_policy.hpp>
#include <ksets/clustering/histogram_preselect_policy.hpp>
#include <ksets/clustering/histogram_empty_policy.hpp>
#include <ksets/clustering/histogram_empty_greedyfix.hpp>
#include <ksets/clustering/histogram_empty_keep.hpp>
#include <ksets/hashing/murmur_hash.hpp>
#include <ksets/distance/jaccard.hpp>

#include <ksets/types.hpp>
#include <ksets/telemetry.hpp>
#include <ksets/result.hpp>
#include <string> 
#include <format>

#include <numeric>
#include <iostream>
#include <algorithm>
#include <random>
#include <ranges>
#include <vector>
#include <chrono>
//using Histogram = std::unordered_map<std::string, int, ksets::MurmurHash>;

namespace ksets
{

enum class EndReason {
  DistanceSumStagnated,
  DistanceIncreased,
  IterationLimitReached,
  Interupt,
};     

/*struct ClusterResult {
    std::vector<label_t> labels;
    std::vector<std::vector<hist_count_t>> histograms;
    std::vector<double> sumd;
};*/

struct AlgorithmStart {
    static constexpr telemetry::TelemetryLevel level =
        telemetry::TelemetryLevel::basic;

    size_t n_items;
    size_t k;
    size_t max_iter;
    bool histograms_provided;
};

struct IterationMetrics {
    static constexpr telemetry::TelemetryLevel level =
        telemetry::TelemetryLevel::basic;

    size_t iter;
    double objective;
    double delta;
    std::chrono::high_resolution_clock::time_point timestamp;
};



template <class HistogramCollection>
std::vector<std::vector<hist_count_t>> to_hist_output_format(
    const HistogramCollection& hist_vec,
    int max_hist_length
){
    std::vector<std::vector<hist_count_t>> out;
    out.reserve(hist_vec.size());

    for (size_t i = 0; i < hist_vec.size(); ++i) {
        out.push_back(hist_vec[i].values(max_hist_length));
    }
    return out;
}
    /*template <typename Updater>
    concept DeltaUpdater = requires(Updater u) {
        typename Updater::delta_capability;
        { u.get_changes() };
    };

    template <typename Preselector>
    concept DeltaPreselector = requires(Preselector p, auto& changes) {
        typename Preselector::delta_capability;
        { p.apply_changes(changes) };
    };

    template <typename Preselector, typename Updater>
    void maybe_apply_delta(Preselector& pre, Updater& updater, const std::vector<label_t>& labels) {
        if constexpr (DeltaUpdater<Updater> && DeltaPreselector<Preselector>) {
            pre.apply_changes(updater.get_changes());
        } else {
            pre.update(labels);
        }
    }
    */

// Telemetry Events that can bew interceptet by a suitable Sink
// They are compiled out, when a NullSink is used.

struct HistogramStats {
    double avg;                     // average histogram length
    size_t median;                  // median histogram length
    double sum_distances;           // total distance sum
    std::vector<size_t> lengths;    // raw histogram lengths
    std::vector<double> deciles;
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

HistogramStats compute_hist_stats(const auto& histograms) {
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

template <class HistogramCollection, 
          template<class> class HistogramUpdatePolicy,
          template<class, class> class HistogramPreselectPolicy,
          template<class> class DistancePolicy = distance::JaccardDistance,
          class EmptyClusterPolicy = EmptyClusterKeep,
          telemetry::TelemetrySink Sink = telemetry::NullSink>
ClusterResult cluster_cpp(
        const std::vector<std::vector<token_t>> & data,
        int K,
        int max_hist_length,
        HistogramCollection& histograms,
        int max_iter = 50, 
        int seed = 42,
        Sink sink = {}){
    
    enum EndReason reason = EndReason::DistanceSumStagnated;
    bool histograms_provided = ! histograms.empty();
    
    
    DistancePolicy<HistogramCollection> distance(histograms);
 

    if (max_iter == 0)
        max_iter = 50;
    
    telemetry::emit(sink, [&]{
        return AlgorithmStart{
            data.size(),
            size_t(K),
            size_t(max_iter),
            histograms_provided
        };
    });
    
    std::vector<label_t> labels_cur(data.size());
    std::vector<label_t> labels_prev(data.size());
    std::mt19937_64 rng(seed);
    
    // Randomly assign labels
    auto randomizer = ksets::hashing::MurmurHash(seed);
    std::vector<size_t> indices(data.size());
    std::iota(indices.begin(), indices.end(), size_t{});
    std::shuffle(indices.begin(), indices.end(), rng);
    for (size_t j = 0; j < data.size(); j++){
        labels_cur[j] = indices[j] % K;
    }
    // Build initial histograms if not provided 
    if (! histograms_provided){
        for (cluster_t i = 0; i < K; i++) {
            size_t index = indices[i];
            labels_cur[index] = i; 
            for (auto const& token : data[index]) {
                histograms.increment(i, token);
            }
        }
        histograms_provided = true; 
    }
    HistogramPreselectPolicy<HistogramCollection, decltype(distance)> histPreselector(histograms, data, distance);
    if constexpr (SupportsQuickLabelEstimate<decltype(histPreselector)>){
        for (size_t i=0; i<data.size(); i++){
            labels_cur[i] = histPreselector.estimate_best_cluster(data[i]);
        }

    }
    HistogramUpdatePolicy<HistogramCollection> histUpdater(histograms, data, histograms_provided);
    EmptyClusterPolicy emptyClusterFix(labels_cur, K);

    // Zero out data structurs
    double sdth_old = 0.0; // Previous Iteration Sum of distances 
    double sdth = std::numeric_limits<double>::max(); // Current Iteration Sum of distance (Termination Criterion)
    std::vector<double> sumd_prev(K,0); // Per cluster sums of contained elements of last iteration
    std::vector<double> sumd(K,0); // Current iteration distances 
    auto const & hist_sums = histograms.totals(); // Sums of observed elements per cluster
    
    size_t j = 0; // Main Iteration Counter
    
     
    uint64_t total_hists_touched = 0;
    uint64_t total_tokens = 0; 
    size_t report_interval=5000; 
    size_t next_report = report_interval;
    //Main Loop
    while ((std::abs(sdth_old - sdth) > 0.00001 ) && (j < max_iter)){
        histUpdater.before_iteration(j);
        distance.begin_iteration(j);
        histPreselector.before_iteration(j, labels_cur);
        emptyClusterFix.begin_iteration(j);
        sdth_old = sdth;
        labels_prev = labels_cur;
        sumd_prev = sumd;
        //Reset Distance Sums per cluster
        std::fill(sumd.begin(), sumd.end(), 0);
        // and total sum
        sdth = 0; 
        //4.1 Compute distances & labels
        // Iterating over data rows vvv
        for (size_t i = 0; i < data.size(); i++) {
            distance.begin_row();
            label_t best_cluster; 
            label_t curr_cluster = labels_cur[i];
            size_t num_tokens = data[i].size();
            auto histRange = histPreselector.get_candidate_histograms(data[i], i);
            // Telemetry for savings using preselect strategy
            
            total_hists_touched += uint64_t(histRange.size()) * uint64_t(num_tokens);
            total_tokens += num_tokens;
            if (--next_report == 0 || i+1 == data.size()) {
                telemetry::emit(sink, [&]{
                    return PreselectSavingsTelemetry{uint64_t(total_tokens), uint64_t(K), total_hists_touched};
                });
                total_hists_touched = uint64_t(0);
                total_tokens = uint64_t(0);
                next_report = report_interval;
            }  // End of Telemetry Report for Preselect Strategy  
            
            //for (auto const& token : data[i]) { // accumulate distance measures used
            //        distance.accumulate_token(token, histRange);
            //}
            
            distance.accumulate_token(data[i], histRange); // accumulate distance measures used
         
            // return finalized distances 
            auto const& dists = distance.finalize();
            // Emit Trace Telemetry for distances computed 
            telemetry::emit(sink, [&]{
                    return DistanceMeasureTelemetry{j, i, dists};
                });
            //emptyClusterFix.register_distances(i, dists);
            
            
            best_cluster = std::distance(dists.begin(),
                                         std::min_element(dists.begin(), dists.end()));
            
            
            auto selection = curr_cluster;
            telemetry::emit(sink, [&]{
                    return LabelSelectTelemetry{j, i, curr_cluster, best_cluster, dists[curr_cluster], dists[best_cluster]};
                });
            if ((j == 0) || ((dists[curr_cluster] - dists[best_cluster]) > 0.0 )){
                histUpdater.register_move(i,curr_cluster, best_cluster);
                selection = best_cluster;
            }
            emptyClusterFix.register_move(i, curr_cluster, selection, dists[selection]);
            labels_cur[i] = selection;
            sumd[selection] += dists[selection];
            sdth += dists[selection];    
            
        } // Iteration over Data rows

        //4.2 Stop if error increases
        if (sdth > sdth_old) {
            //std::cout << std::format("Error Increased: sdth={0:0.6f} > sdth_old={1:0.6f}\n", sdth, sdth_old);
            reason = EndReason::DistanceIncreased; 
            sumd = sumd_prev;
            //reverse histogram changes
            histUpdater.rollback(labels_cur);
            labels_cur = labels_prev;
            auto now = std::chrono::high_resolution_clock::now();
            ++j;
            telemetry::emit(sink, [&]{
            double delta = (j == 0)
                ? std::numeric_limits<double>::quiet_NaN()
                : std::abs(sdth_old - sdth);
            return IterationMetrics{j, sdth, delta, now };
            });
            sdth = sdth_old;
            break;
        }
        if (j==0 && histograms_provided){
            histograms.zero_out();
        } 
        //4.3. Recompute histograms by applying histogram changes using data rows
        emptyClusterFix.execute_fix(labels_cur);
        histUpdater.commit(labels_cur);
        histPreselector.update(labels_cur);
        histograms.totals();
        //4.4 increment counter
        
        auto now = std::chrono::high_resolution_clock::now();
        telemetry::emit(sink, [&]{
            double delta = (j == 0)
                ? std::numeric_limits<double>::quiet_NaN()
                : std::abs(sdth_old - sdth);
            return IterationMetrics{j, sdth, delta, now };
        });


        ++j;
    }

    telemetry::emit(sink, [&]{ 
        return RunSummary{j,sdth, reason, compute_hist_stats(histograms)};
    });
    
    
    // 5. Results


    return {labels_cur, j, sdth};
}


}





    

