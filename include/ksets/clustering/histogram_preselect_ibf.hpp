#pragma once
#include <ksets/clustering/histogram_preselect_policy.hpp>
#include <ksets/histogram/collection_iterator.hpp>
#include <ksets/types.hpp>
#include <hibf/interleaved_bloom_filter.hpp> // for bin_index, interleaved_bloom_filter, bin_count, bin_size, hash_...
#include <hibf/misc/print.hpp>               // for print, print_t
#include <hibf/misc/bit_vector.hpp>
#include <numeric>
#include <vector>


namespace ksets {

    template <typename HistogramCollection>
    class HistPreselectIBF
        : public HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection>, HistogramCollection>
    {
    public:
        using Base = HistogramPreselectPolicy<HistPreselectIBF<HistogramCollection>,
                            HistogramCollection>;
        using Range = histogram::CollectionRange<HistogramCollection>;
        using Base::Base;
        
        explicit HistPreselectIBF(HistogramCollection& hist,
                            const std::vector<std::vector<token_t>>& data)
            : Base(hist, data),  
            hists_(hist), data_(data), indices_(hist.size()), 
            ibf_(seqan::hibf::bin_count{hist.size()},
                 seqan::hibf::bin_size{128u},
                 seqan::hibf::hash_function_count{3u})
        {
            std::iota(indices_.begin(), indices_.end(), 0);
            cluster_agent_ = ibf_.containment_agent();
        }
        void before_iteration_impl(size_t it_idx) {
            if(it_idx == 0){
                for (size_t i = 0; i < hists_.size(); ++i) {
                    for (const auto& key :hists_[i].keys())
                        ibf_.emplace(key, seqan::hibf::bin_index{i});
                }
            }
        }

        Range get_candidate_histograms_impl(const std::vector<token_t>& data_row) {
            seqan::hibf::bit_vector bit_results(0, hists_.size());
            std::vector<size_t> results;
            results.reserve(hists_.size());
            for (const auto& token: data_row){
                auto & res = cluster_agent_.bulk_contains(token);
                bit_results |= res; 
            }
            seqan::hibf::print(bit_results);
            // create index from bitvector? 
            bit_vector_indices(bit_results, std::back_inserter(results));
            return hists_.get_range(results);
        }


        void update_impl(std::vector<int>& labels_cur) {
        }



    private:
        seqan::hibf::interleaved_bloom_filter ibf_;
        seqan::hibf::interleaved_bloom_filter::containment_agent_type cluster_agent_;
        HistogramCollection& hists_;
        std::vector<size_t> indices_;
        const std::vector<std::vector<token_t>>& data_;

        template <typename BitVector, typename OutputIt>
        void bit_vector_indices(BitVector const & bv, OutputIt out){
            using chunk_type = std::remove_cv_t<std::remove_pointer_t<decltype(bv.data())>>;
            constexpr size_t chunk_bits = sizeof(chunk_type) * CHAR_BIT;

            size_t const nbits = bv.size();
            size_t const nchunks = (nbits + chunk_bits - 1) / chunk_bits;

            chunk_type const * chunks = bv.data();

            for (size_t c = 0; c < nchunks; ++c)
            {
                chunk_type x = chunks[c];

                // Mask unused bits in last chunk
                if (c + 1 == nchunks)
                {
                    size_t const valid = nbits - c * chunk_bits;
                    if (valid < chunk_bits)
                        x &= (chunk_type{1} << valid) - 1;
                }

                while (x)
                {
                    unsigned bit = std::countr_zero(x); 
                    *out++ = c * chunk_bits + bit;
                    x &= x - 1;
                }
            }
        }
    };

} // namespace ksets
