#pragma once
#include <ranges>
#include <ksets/options.hpp>
#include <ksets/result.hpp>
#include <ksets/clustering/detail/cluster_dispatch.hpp>
#include <ksets/telemetry.hpp>

namespace ksets {

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


}// namespace ksets 