#include <scripts/generate_icds.hpp>
#include <fstream>

int main() {

    constexpr int NUM_GROUPS = 500;
    constexpr int NUM_PATIENTS = 38451;
    constexpr double DENSITY = 0.015;

    std::mt19937_64 rng(123);

    std::vector<genicds::Group> groups;

    for(int i=0;i<NUM_GROUPS;i++) {
        groups.push_back({
            uint64_t(i),
            uint64_t(DENSITY * double(UINT64_MAX))
        });
    }

    std::ofstream out("icd_38451_500_large_sets.txt");
    out.rdbuf()->pubsetbuf(nullptr, 1<<20); // 1MB buffer

    for (uint64_t pid = 0; pid < NUM_PATIENTS; ++pid) {

        auto codes = genicds::generate_patient(groups,rng);

        out << '';//pid;

        for (auto c : codes)
            out << ' ' << c;

        out << '\n';
    }
}