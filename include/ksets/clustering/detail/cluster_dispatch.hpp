#pragma once 
#include <ranges>

#include <ksets/options.hpp>
#include <ksets/result.hpp>
#include <ksets/clustering/clustering.hpp>
#include <ksets/clustering/detail/all_algo_policies.hpp>
#include <ksets/histogram/collection_map.hpp>
#include <ksets/distance/jaccard.hpp>
#include <ksets/hashing/all_hash_fun.hpp>

namespace ksets::detail {


    template<
        typename Hash,
        template<class> class Distance,
        template<class, class> class Updater,
        template<class, class> class Preselect,
        
        std::ranges::forward_range Data,
        typename Telemetry
    >
    ClusterResult run(
            const Data& data,
            const ClusterOptions& options,
            const Telemetry& telemetry){
        
        auto hash = Hash(options.seed);

        auto hash_policy = hashing::HashPolicy<Hash>{hash};

        histogram::MapHistogramCollection collection{
            options.clusters,
            hash_policy
        };

        return cluster_cpp<
            decltype(collection),
            Updater,
            Preselect,
            Distance
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

    template<
        typename Hash,
        template<class> class Distance, 
        template<class, class> class Updater, 
        std::ranges::forward_range Data,
        typename Telemetry
    >
    ClusterResult dispatch_preselect(
            const Data& data,
            const ClusterOptions& options,
            const Telemetry& telemetry){
        switch (options.preselect){
            case PreselectStrategy::none:
                return run<Hash, Distance, Updater, HistPreselectNothing>(
                    data, options, telemetry);

            case PreselectStrategy::ibf:
                return run<Hash, Distance, Updater, HistPreselectIBF>(
                    data, options, telemetry);
        }
        std::unreachable();
    }

    template<
        typename Hash,
        template<class> class Distance, 
        std::ranges::forward_range Data,
        typename Telemetry
    >
    ClusterResult dispatch_rebuild(
            const Data& data,
            const ClusterOptions& options,
            const Telemetry& telemetry){
        switch (options.rebuild){
            case HistogramRebuild::full:
                return dispatch_preselect<Hash, Distance, HistUpdaterKsetsAuthors>(
                    data, options, telemetry);

            case HistogramRebuild::update:
                return dispatch_preselect<Hash, Distance, HistUpdaterMemorizeChanges>(
                    data, options, telemetry);
        }
        std::unreachable();
    }

    template<
        typename Hash,
        std::ranges::forward_range Data,
        typename Telemetry
    >
    ClusterResult dispatch_distance(
            const Data& data,
            const ClusterOptions& options,
            const Telemetry& telemetry){
        switch (options.distance){
            case DistanceType::jaccard:
                return dispatch_rebuild<Hash, distance::JaccardDistance>(
                    data, options, telemetry);

            /*case DistanceType::cosine:
                return dispatch_rebuild<Hash, distance::CosineDistance>(
                    data, options, telemetry);*/
        }
        std::unreachable();
    }

    template<
        std::ranges::forward_range Data,
        typename Telemetry
    >
    ClusterResult cluster_dispatch(
            const Data& data,
            const ClusterOptions& options,
            const Telemetry& telemetry){
        switch (options.hash){
            case HashStrategy::murmur:
                return dispatch_distance<hashing::MurmurHash>(
                    data, options, telemetry);

            case HashStrategy::prehashed:
                return dispatch_distance<hashing::IdentityHash>(
                    data, options, telemetry);
        }
        std::unreachable();
    }
    
}