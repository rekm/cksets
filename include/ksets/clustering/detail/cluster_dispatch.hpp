#pragma once 
#include <ranges>

#include <ksets/options.hpp>
#include <ksets/result.hpp>
#include <ksets/clustering/clustering.hpp>
#include <ksets/clustering/detail/all_algo_policies.hpp>
#include <ksets/clustering/detail/collection_dispatch.hpp>
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
            const Telemetry& telemetry,
            uint swap_iter,
            histogram::IHistogramCollection* user_collection = nullptr){
        
        return ksets::detail::dispatch_collection(options, [&](auto&& collection) {
                using TargetConcreteType = std::decay_t<decltype(collection)>;

                if (user_collection != nullptr) {
                    // WORKFLOW B: User provided an existing collection. Unpack and downcast it!
                    auto* concrete_ptr = dynamic_cast<TargetConcreteType*>(user_collection);
                    if (!concrete_ptr) {
                        throw std::runtime_error("ClusterOptions do not match the internal layout of the provided collection!");
                    }
                    if (swap_iter == 0){
                        return cluster_cpp<TargetConcreteType, Updater, Preselect, Distance>(
                            data, options.clusters, options.max_hist_length,
                            *concrete_ptr, // Use user's pre-populated instance
                            options.max_iterations, options.seed, telemetry
                        );
                    } else {
                        return cluster_swap_cpp<TargetConcreteType, Updater, Preselect, Distance>(
                            data, options.clusters, options.max_hist_length,
                            *concrete_ptr, // Use user's pre-populated instance
                            swap_iter, options.max_iterations, options.seed, telemetry
                        );

                    }
                    
                } else {
                    if (swap_iter == 0){
                        return cluster_cpp<TargetConcreteType, Updater, Preselect, Distance>(
                            data, options.clusters, options.max_hist_length,
                            collection, // Use freshly allocated instance
                            options.max_iterations, options.seed, telemetry
                        );
                    } else {
                        return cluster_swap_cpp<TargetConcreteType, Updater, Preselect, Distance>(
                            data, options.clusters, options.max_hist_length,
                            collection, // Use freshly allocated instance
                            swap_iter, options.max_iterations, options.seed, telemetry
                        );

                    }
                    
                }
            });
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
            const Telemetry& telemetry,
            uint swap_iter, 
            histogram::IHistogramCollection* user_collection = nullptr){
        switch (options.preselect){
            case PreselectStrategy::none:
                return run<Hash, Distance, Updater, HistPreselectNothing>(
                    data, options, telemetry, swap_iter, user_collection);

            case PreselectStrategy::ibf:
                return run<Hash, Distance, Updater, HistPreselectIBF>(
                    data, options, telemetry, swap_iter, user_collection);
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
            const Telemetry& telemetry,
            uint swap_iter, 
            histogram::IHistogramCollection* user_collection = nullptr){
        switch (options.rebuild){
            case HistogramRebuild::full:
                return dispatch_preselect<Hash, Distance, HistUpdaterKsetsAuthors>(
                    data, options, telemetry, swap_iter, user_collection);

            case HistogramRebuild::update:
                return dispatch_preselect<Hash, Distance, HistUpdaterMemorizeChanges>(
                    data, options, telemetry, swap_iter, user_collection);
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
            const Telemetry& telemetry,
            uint swap_iter,
            histogram::IHistogramCollection* user_collection = nullptr){
        switch (options.distance){
            case DistanceType::jaccard:
                return dispatch_rebuild<Hash, distance::JaccardDistance>(
                    data, options, telemetry, swap_iter, user_collection);

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
            histogram::IHistogramCollection* user_collection,
            const Telemetry& telemetry, 
            uint swap_iter=0){
        switch (options.hash){
            case HashStrategy::murmur:
                return dispatch_distance<hashing::MurmurHash>(
                    data, options, telemetry, swap_iter, user_collection);

            case HashStrategy::prehashed:
                return dispatch_distance<hashing::IdentityHash>(
                    data, options, telemetry, swap_iter, user_collection);
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
            const Telemetry& telemetry, 
            uint swap_iter=0){
        switch (options.hash){
            case HashStrategy::murmur:
                return dispatch_distance<hashing::MurmurHash>(
                    data, options, telemetry, swap_iter);

            case HashStrategy::prehashed:
                return dispatch_distance<hashing::IdentityHash>(
                    data, options, telemetry, swap_iter);
        }
        std::unreachable();
    }
    
}