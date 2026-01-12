

#pragma once
#include <string_view>
#include <unordered_map>
#include <deque>
#include <ksets/token_encoder/encoder.hpp>

namespace ksets::token_encoder {

class ReversibleInterning {
public:
    token_t encode(std::string_view code) {
        auto it = forward_.find(code);
        if (it != forward_.end())
            return it->second;

        token_t id = next_id_++;
        storage_.emplace_back(code);
        std::string_view view = storage_.back();
        forward_.emplace(view, id);
        reverse_.emplace(id, view);
        return id;
    }

    std::string_view decode(token_t id) const {
        return reverse_.at(id);
    }

private:
    token_t next_id_{0};

    // owns memory
    std::deque<std::string> storage_;

    std::unordered_map<std::string_view, token_t> forward_;
    std::unordered_map<token_t, std::string_view> reverse_;
};

}