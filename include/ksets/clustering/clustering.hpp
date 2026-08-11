
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
#include <ksets/clustering/algo_info.hpp>
#include <ksets/telemetry/base.hpp>
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
    


template <class HistogramCollection, 
          template<class, class> class HistogramUpdatePolicy,
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
    HistogramUpdatePolicy<HistogramCollection, decltype(sink)> histUpdater(histograms, data, sink, histograms_provided);
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





    

