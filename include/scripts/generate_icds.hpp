#pragma once

#include <random>
#include <vector>
#include <unordered_set>


namespace genicds {
    
    constexpr uint32_t GROUP_BITS = 10;
    constexpr uint32_t CODE_BITS  = 32 - GROUP_BITS;

    constexpr uint32_t CODE_MASK  = (1u << CODE_BITS) - 1;

    uint32_t make_code(uint32_t group_id, uint32_t local_code){
        return (group_id << CODE_BITS) | (local_code & CODE_MASK);
    }

    uint32_t extract_group(uint32_t code){
        return code >> CODE_BITS;
    }

    uint32_t extract_local(uint32_t code){
        return code & CODE_MASK;
    }




    struct Group {
        uint64_t id;
        uint64_t thresh;
    };

    uint64_t fmix64(uint64_t k) {
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53ULL;
        k ^= k >> 33;
        return k;
    }

    uint64_t hash(uint64_t a, uint32_t b) {
        return fmix64(a ^ (uint64_t(b)<<32));
    }

    const uint32_t TOTAL_CODES = 100000;
    const double density = 0.01;


    struct ZipfSampler {
        std::vector<double> cdf;
        uint32_t n;

        ZipfSampler(uint32_t n_codes, double alpha)
                : n(n_codes){
            cdf.resize(n);

            double sum = 0;
            for(uint32_t i=1;i<=n;i++)
                sum += 1.0 / std::pow(i, alpha);

            double acc = 0;
            for(uint32_t i=1;i<=n;i++) {
                acc += (1.0 / std::pow(i, alpha)) / sum;
                cdf[i-1] = acc;
            }
        }

        uint32_t sample(std::mt19937_64& rng) const {
                std::uniform_real_distribution<double> u(0,1);
                double x = u(rng);

                return std::lower_bound(cdf.begin(), cdf.end(), x) - cdf.begin();
        }
    };


    std::vector<uint32_t>
    generate_patient(const std::vector<Group>& groups,
                    std::mt19937_64& rng){
        std::uniform_int_distribution<int> ng(1,3);
        std::uniform_real_distribution<double> u(0,1);

        int k = ng(rng);

        std::unordered_set<uint32_t> codes;
        codes.reserve(4000);

        size_t expected = TOTAL_CODES * density;

        for(int i=0;i<k;i++) {

            const Group& g = groups[rng()%groups.size()];

            for(size_t j=0;j<expected;j++) {

                uint32_t c = hash(g.id,j) % TOTAL_CODES;
                c = make_code(g.id, c);

                if(u(rng) < 0.8)
                    codes.insert(c);
            }
        }

        return {codes.begin(), codes.end()};
    }

    std::vector<uint16_t>
    generate_patient_powerlaw(
            const std::vector<Group>& groups,
            const ZipfSampler& zipf,
            std::mt19937_64& rng){
        std::uniform_int_distribution<int> ng(1,3);

        int k = ng(rng);

        std::unordered_set<uint16_t> codes;
        codes.reserve(4096);

        for(int i=0;i<k;i++){
            const Group& g = groups[rng() % groups.size()];

            size_t group_size =
                1500 + (hash(g.id, i) % 1500); // 1500–3000 codes

            for(size_t j=0;j<group_size;j++){
                uint32_t c = zipf.sample(rng);
                c = make_code(g.id, c);
                // group-specific filter to create structure
                if(hash(g.id, c) < g.thresh)
                    codes.insert(c);
            }
        }

        return {codes.begin(), codes.end()};
    }

    
}

