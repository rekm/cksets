#pragma once
#include <ranges>
#include <ksets/options.hpp>
#include <ksets/result.hpp>
#include <ksets/clustering/detail/cluster_dispatch.hpp>
#include <ksets/clustering/detail/collection_dispatch.hpp>
#include <ksets/telemetry/base.hpp>

namespace ksets {

/**
 * @brief Workflow 1: End-to-end clustering.
 * Generates an internal histogram collection matching the options automatically.
 */
template<
    std::ranges::forward_range Data,
    typename Telemetry = telemetry::NullSink
>
ClusterResult cluster(
    Data const& data,
    ClusterOptions const& options = {},
    Telemetry telemetry = {})
{
    return detail::cluster_dispatch(data, options, telemetry);
}

/**
 * @brief Factory: Provides an empty histogram collection matching the configuration.
 * Users can fill this handle sequentially from files, streams, or Python/NumPy loops.
 */
inline std::unique_ptr<histogram::IHistogramCollection> histograms(
    ClusterOptions const& options = {}
){
    // Resolves switches internally and returns the unique_ptr to the abstract top layer
    return detail::make_collection(options);
}

/**
 * @brief Workflow 2: Clustering from a pre-populated user histogram.
 * Unpacks the virtual collection via internal dynamic downcasting back to fast CRTP code.
 */
template<
    std::ranges::forward_range Data,
    typename Telemetry = telemetry::NullSink
>
ClusterResult cluster(
    Data const& data,
    histogram::IHistogramCollection& user_histograms,
    ClusterOptions const& options = {},
    Telemetry telemetry = {})
{
    return detail::cluster_dispatch(data,options,&user_histograms, telemetry);
}

template<
    std::ranges::forward_range Data, 
    typename Telemetry = telemetry::NullSink>
ClusterResult cluster(
    const Data& data,
    const std::unique_ptr<histogram::IHistogramCollection>& user_collection_ptr, 
    const ClusterOptions& options = {},
    Telemetry telemetry = {})
{
    if (!user_collection_ptr) {
        throw std::invalid_argument("Provided histogram collection pointer is null.");
    }
    
    return cluster(data, *user_collection_ptr, options, telemetry);
}


}// namespace ksets 