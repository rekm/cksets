#pragma once
#include <ksets/types.hpp>
#include <vector>
#include <string_view>

namespace ksets::token_encoder {

    template<class Encoder>
    std::vector<std::vector<token_t>>
    encode_rows(const std::vector<std::vector<std::string>>& rows,
                Encoder& encoder)
    {
        std::vector<std::vector<token_t>> out;
        out.reserve(rows.size());

        for (auto const& row : rows) {
            std::vector<token_t> encoded;
            encoded.reserve(row.size());
            for (auto const& s : row)
                encoded.push_back(encoder.encode(s));
            out.push_back(std::move(encoded));
        }
        return out;
    }



    template<class Encoder>
    concept TokenEncoder =
        requires(Encoder enc, std::string_view s, token_t t) {
            { enc.encode(s) } -> std::same_as<token_t>;
            { enc.decode(t) } -> std::same_as<std::string_view>;
        };

}