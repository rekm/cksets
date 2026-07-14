#pragma once

namespace ksets::telemetry{
    enum class TelemetryCategory {
        timing,
        histogram,
        movement,
        summary,
        candidate_selection
    };

    enum class TelemetryLevel {
        none,
        basic,
        debug,
        trace,
        full_trace,
    };

    struct NullSink {
        static constexpr TelemetryLevel level = TelemetryLevel::none;

        template<class E>
        constexpr void handle(const E&) const noexcept {}
    };

    template<class S>
    concept TelemetrySink =
        requires {
            { S::level } -> std::convertible_to<TelemetryLevel>;
        };

    template<TelemetrySink Sink, class EventFactory>
    constexpr void emit(Sink& sink, EventFactory&& make_event){
    using Event = std::invoke_result_t<EventFactory>;

    if constexpr (Event::level <= Sink::level)
        sink.handle(make_event());
    }

    

}