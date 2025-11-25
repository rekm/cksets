#pragma once

namespace ksets {
namespace histogram {

template <class Derived>
class HistogramBase {
public:
    void add_token(std::string const& token) {
        derived().increment(token);
    }
    
    auto total() const {
        return static_cast<const Derived*>(this)->total();
    }

    int operator[](std::string const& token) const {
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
} // namespace mylib