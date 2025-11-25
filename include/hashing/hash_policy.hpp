
namespace ksets {
namespace hashing {

template <typename HashFunc>
struct HashPolicy {
    HashFunc hash_func;

    explicit HashPolicy(HashFunc h = {}) : hash_func(h) {}

    std::size_t operator()(const std::string& key) const noexcept {
        return hash_func(key);  // forward to underlying hash
    }

    std::size_t hash(std::string const& key) const noexcept {
        return hash_func(key);
    }
};

}
}

 