#pragma once 
#include <ranges>

#include <ksets/options.hpp>
#include <ksets/result.hpp>
#include <ksets/clustering/clustering.hpp>
#include <ksets/clustering/histogram_update_memorize_changes.hpp>
#include <ksets/clustering/histogram_update_ksets_authors.hpp>
#include <ksets/clustering/histogram_preselect_nothing.hpp>
#include <ksets/clustering/histogram_preselect_ibf.hpp>
#include <ksets/histogram/collection_map.hpp>

namespace ksets::detail {
    template<
        std::ranges::forward_range Data,
        typename Telemetry
    >
    ClusterResult cluster_dispatch(
            const Data& data,
            const ClusterOptions& options, 
            const Telemetry& telemetry){
        
        auto hash_policy =
            hashing::HashPolicy<hashing::MurmurHash>(
                hashing::MurmurHash(options.seed));


        histogram::MapHistogramCollection collection(
            options.clusters,
            hash_policy);


        if(options.rebuild == HistogramRebuild::full){
            if(options.preselect == PreselectStrategy::none){
                return cluster_cpp<
                    decltype(collection),
                    HistUpdaterKsetsAuthors,
                    HistPreselectNothing
                >(
                    data,
                    options.clusters,
                    options.max_hist_length,
                    collection,
                    options.max_iterations,
                    options.seed,
                    telemetry
                );
            }


            if(options.preselect == PreselectStrategy::ibf){
                return cluster_cpp<
                    decltype(collection),
                    HistUpdaterKsetsAuthors,
                    HistPreselectIBF
                >(
                    data,
                    options.clusters,
                    options.max_hist_length,
                    collection,
                    options.max_iterations,
                    options.seed,
                    telemetry
                );
            }
        } else if (options.rebuild == HistogramRebuild::update) {
            if(options.preselect == PreselectStrategy::none){
                return cluster_cpp<
                    decltype(collection),
                    HistUpdaterMemorizeChanges,
                    HistPreselectNothing
                >(
                    data,
                    options.clusters,
                    options.max_hist_length,
                    collection,
                    options.max_iterations,
                    options.seed,
                    telemetry
                );
            }


            if(options.preselect == PreselectStrategy::ibf){
                return cluster_cpp<
                    decltype(collection),
                    HistUpdaterMemorizeChanges,
                    HistPreselectIBF
                >(
                    data,
                    options.clusters,
                    options.max_hist_length,
                    collection,
                    options.max_iterations,
                    options.seed,
                    telemetry
                );
            }
        }
    }
}