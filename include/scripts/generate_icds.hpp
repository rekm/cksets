#pragma once

#include <random>
#include <vector>
#include <unordered_set>


namespace genicds {
    
    using pcode_t = uint32_t;
    using gcode_t = uint32_t;

    constexpr pcode_t GROUP_BITS = 10;
    constexpr pcode_t CODE_BITS  = 32 - GROUP_BITS;

    constexpr pcode_t CODE_MASK  = (1u << CODE_BITS) - 1;

    pcode_t make_code(uint32_t group_id, uint32_t local_code){
        return (group_id << CODE_BITS) | (local_code & CODE_MASK);
    }

    pcode_t extract_group(pcode_t code){
        return code >> CODE_BITS;
    }

    pcode_t extract_local(pcode_t code){
        return code & CODE_MASK;
    }




    struct Group {
        gcode_t id;
        double inner_jaccard;
        double density; 
        size_t exp_size;
    };

    struct Sample { 
        std::vector<pcode_t> codes;
        gcode_t primary_g_id;
        size_t primary_g_size;
        std::vector<gcode_t> sub_gids;
        std::vector<size_t> sub_g_sizes;
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

    const uint32_t TOTAL_CODES = 2000000;
    const double density = 0.0006;


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


    Sample
    generate_patient(const std::vector<Group>& groups,
                    std::mt19937_64& rng){
        std::uniform_int_distribution<int> ng(1,3);
        std::uniform_real_distribution<double> u(0,1);

        const Group& primary = groups[rng() % groups.size()];
        int k = ng(rng);
        k=1;
        double dropout_rate = 2.0*primary.inner_jaccard / (primary.inner_jaccard  + 1.0);
        std::unordered_set<pcode_t> codes;
        codes.reserve(4000);
        size_t primary_size = primary.exp_size / dropout_rate;
        size_t primary_universe = primary_size / primary.density;
        std::vector<gcode_t> subgroup_ids;
        std::vector<size_t> subgroup_sizes;              
        for(int i=0;i<k;i++) {

            const Group& g = (i == 0) ? primary : groups[rng() % groups.size()];
            size_t expected = primary_universe * primary.density;
            if(g.id != primary.id){
                expected /= 4;
                subgroup_sizes.push_back(expected);
                subgroup_ids.push_back(g.id);
            }  
            for(size_t j=0;j<expected;j++) {

                uint32_t c = hash(g.id,j) % (primary_universe);
                double keep = (g.id == primary.id) ? dropout_rate : 0.25;
                c = make_code(g.id, c);
                if(u(rng) < keep)
                    codes.insert(c);
            }
        }

        return {
            {codes.begin(), codes.end()}, 
            primary.id,
            primary_size,
            {subgroup_ids.begin(),subgroup_ids.end()},
            {subgroup_sizes.begin(),subgroup_sizes.end()}};
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
            double dropout_rate = 2.0*g.inner_jaccard / (g.inner_jaccard  + 1.0);
            size_t group_size = g.exp_size / dropout_rate;
                //1500 + (hash(g.id, i) % 1500); // 1500–3000 codes

            for(size_t j=0;j<group_size;j++){
                uint32_t c = zipf.sample(rng);
                c = make_code(g.id, c);
                // group-specific filter to create structure
                if(hash(g.id, c) < dropout_rate)
                    codes.insert(c);
            }
        }

        return {codes.begin(), codes.end()};
    }

    
}

