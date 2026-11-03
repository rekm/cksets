#pragma once
#include <ksets/types.hpp>
#include <ksets/histogram/base.hpp>
#include <ksets/histogram/collection_iterator.hpp>
#include <vector>
#include <span>
#include <iostream>

namespace ksets::histogram {

    class IHistogramCollection {
        public:
            virtual ~IHistogramCollection() = default;
            
            // Define the uniform API your users need to fill from files/NumPy
            virtual size_t size() const = 0;
            virtual bool increment(size_t cluster, const token_t& token) = 0;
            virtual bool decrement(size_t cluster, const token_t& token) = 0;
            virtual void set(size_t cluster, const token_t& token, hist_count_t value) = 0;  
    };


    template <typename Derived>
    class BaseHistogramCollection : public IHistogramCollection {
    protected:
        Derived& derived() { return static_cast<Derived&>(*this); }
        const Derived& derived() const { return static_cast<const Derived&>(*this); }

    public:
        // Increment token in the collection

        bool increment(size_t cluster, const token_t& token){
            return derived().increment_impl(cluster, token);
        }

        bool decrement(size_t cluster, const token_t& token){
            return derived().decrement_impl(cluster, token);
        }
        

        void zero_out(){
            derived().zero_out();
        }

        Derived clone_empty() const {
            return derived().clone_empty_impl();
        }

        // Get token count for a single histogram (default: first histogram)
        hist_count_t get(const token_t& token) const {
            return derived().get(token);
        }

        void set(size_t cluster, const token_t& token, hist_count_t value) {
            derived().set_impl(cluster, token, value);
        }

        /*auto get_range(const std::vector<size_t>&& indices) {
            //std::cout << "Old Range fired\n";
            Derived* self = static_cast<Derived*>(this);
            return CollectionRange<Derived, std::vector<size_t>>{
                self,
                std::move(indices)
            };
        }*/
        template <typename T>
        auto get_range(std::span<const T> indices) 
            -> CollectionRange<Derived, std::span<const T>>
        {
            Derived* self = static_cast<Derived*>(this);
            return CollectionRange<Derived, std::span<const T>>{self, indices};
        }

        template<std::ranges::forward_range R>
        auto get_range(const R& indices) 
            -> CollectionRange<Derived, std::span<const typename R::value_type>> {
            //std::cout << "New Range fired\n";
            Derived* self = static_cast<Derived*>(this);
            return CollectionRange<Derived, std::span<const typename R::value_type>>{
                self,
                std::span<const typename R::value_type>(indices)
            };
        }

        // Query all histograms in the collection at once
        void get_all(const token_t& token, std::vector<hist_count_t>& out) const {
            if constexpr (requires(const Derived d) { d.query_all(token, out); }) {
                derived().query_all(token, out);
            } else {
                derived().get(token, out);
            }
        }
    };

}