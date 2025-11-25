#pragma once
#include <histogram/base_histogram.hpp>

template <typename Derived>
class BaseHistogramCollection {
protected:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

public:
    // Increment token in the collection
    void add_token(const std::string& token) {
        derived().increment(token);
    }

    // Get token count for a single histogram (default: first histogram)
    int get(const std::string& token) const {
        return derived().get(token);
    }

    // Query all histograms in the collection at once
    std::vector<int> get_all(const std::string& token) const {
        if constexpr (requires(const Derived d) { d.query_all(token); }) {
            return derived().query_all(token);
        } else {
            return {derived().get(token)};
        }
    }
};