#pragma once
#include <ksets/types.hpp>

namespace ksets {
namespace histogram {

template <class Derived>
class HistogramBase {
public:
    void add_token(token_t const& token) {
        derived().increment(token);
    }
    
    void zero_out(){
        derived().zero_out();
    }
    
    auto total() const {
        return static_cast<const Derived*>(this)->total();
    }

    void reserve(size_t s){
        derived().reserve_impl(s); 
    } 
    
    int operator[](token_t const& token) const {
        return derived().get(token);
    }


protected:
    Derived& derived() { 
        return static_cast<Derived&>(*this); 
    }
    Derived const& derived() const { 
        return static_cast<const Derived&>(*this); 
    }
};

} // namespace histogram
} // namespace ksets