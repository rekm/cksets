#pragma once

#include <ksets/options.hpp>
#include <ksets/hashing/all_hash_fun.hpp>
#include <ksets/histogram/all_map_collections.hpp>

namespace ksets::detail{

    
    template <template<typename, typename> typename HistTemplate, typename HashType, typename Op>
    decltype(auto) finalize_collection(const ClusterOptions& options, Op&& op) {
        // Construct the exact configuration type
        using ConcreteCollection = histogram::MapHistogramCollection<hashing::HashPolicy<HashType>, HistTemplate>;
        
        auto hash_instance = HashType(options.seed);
        auto hash_policy = hashing::HashPolicy<HashType>{hash_instance};

        // Instantiate the object and pass it straight into the lambda
        return std::forward<Op>(op)(ConcreteCollection{options.clusters, hash_policy});
    }


    template <template<typename, typename> typename HistTemplate, typename Hash, typename Op> 
    decltype(auto) dispatch_allocator_type(
            const ClusterOptions& options, Op&& op){
        return finalize_collection<HistTemplate, Hash >(options, std::forward<Op>(op));
    }

    template <typename Hash, typename Op> 
    decltype(auto) dispatch_collection_type(
            const ClusterOptions& options, Op&& op){
        switch (options.hist_type){
            case CollectionType::mapGtlFlat:
                switch (options.hist_singletons){
                    case HistogramSingletonStrategy::ignore:
                        return dispatch_allocator_type<histogram::MapGtlFlatmapHistogram, Hash>(options, std::forward<Op>(op));
                    case HistogramSingletonStrategy::remove:
                        return dispatch_allocator_type<histogram::MapGtlFlatmapIgnoreSingletonHistogram, Hash>(options, std::forward<Op>(op));
                    case HistogramSingletonStrategy::track_separate:
                        return dispatch_allocator_type<histogram::MapGtlFlatmapSingletonHistogram, Hash>(options, std::forward<Op>(op));
                }
                std::unreachable();
            case CollectionType::mapRobin:
                return dispatch_allocator_type<histogram::MapRobinHistogram, Hash>(options, std::forward<Op>(op));
        }
        std::unreachable();
    }

    template <typename Op> 
    decltype(auto) dispatch_collection(
            const ClusterOptions& options, Op&& op){
        switch (options.hash){
            case HashStrategy::murmur:
                return dispatch_collection_type<hashing::MurmurHash>(options, std::forward<Op>(op));

            case HashStrategy::prehashed:
                return dispatch_collection_type<hashing::IdentityHash>(options, std::forward<Op>(op));
        }
        std::unreachable();
    }

    inline std::unique_ptr<histogram::IHistogramCollection> make_collection(const ClusterOptions& options){
        return detail::dispatch_collection(options, [](auto&& concrete_collection) {
            using ConcreteType = std::decay_t<decltype(concrete_collection)>;
            // Compiles because MapHistogramCollection indirectly inherits from IHistogramCollection
            return std::unique_ptr<histogram::IHistogramCollection>(
                new ConcreteType(std::forward<decltype(concrete_collection)>(concrete_collection))
            );
        });
    }

}