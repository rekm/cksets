#pragma once
#include <histogram/base.hpp>
#include <histogram/collection_iterator.hpp>

namespace ksets::histogram {

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

        void zero_out(){
            derived().zero_out();
        }

        Derived clone_empty() const {
            return derived().clone_empty_impl();
        }

        // Get token count for a single histogram (default: first histogram)
        int get(const std::string& token) const {
            return derived().get(token);
        }

        auto get_range(const std::vector<size_t>& indices) {
            Derived* self = static_cast<Derived*>(this);
            return CollectionRange<Derived>{
                CollectionIterator<Derived>(self, &indices, 0),
                CollectionIterator<Derived>(self, &indices, indices.size())
            };
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

}