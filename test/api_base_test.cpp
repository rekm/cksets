#include <gtest/gtest.h>

#include "app_test.hpp"
#include <ksets/ksets.hpp>
#include <vector>

struct test_api : public app_test
{};

TEST_F(test_api, simple_api_test){
    std::vector<std::vector<ksets::token_t>> data{{1,2,3,4,5}, {6,2,5,4}, {2,3,4,8}, {2,1,4,7}, {1,2,3,4}};
    ksets::ClusterOptions opts{};
    opts.clusters = 2;
    ksets::ClusterResult res1;
    ksets::ClusterResult res2; 
    res1 = ksets::cluster(data,opts);
    std::cerr << "Objective Score First Run: " << res1.objective << "\n";
    opts.rebuild = ksets::HistogramRebuild::update;
    res2 = ksets::cluster(data,opts);
    EXPECT_EQ(res1.objective, res2.objective) 
        << "Expected matching objective scores between naive rebuild and rebuild with memorization.\n"
        << "Naive full histogram rebuild:  " << res1.objective << "\n"
        << "Memorization update objective: " << res2.objective << "\n";
    EXPECT_EQ(res1.iterations, res2.iterations) 
        << "Expected matching iterations between naive rebuild and rebuild with memorization.\n"
        << "Naive full histogram rebuild:  " << res1.iterations << "\n"
        << "Memorization update objective: " << res2.iterations << "\n";
    opts.preselect = ksets::PreselectStrategy::none;
    res1 = ksets::cluster(data,opts);
    EXPECT_EQ(res1.objective, res2.objective) 
        << "Expected matching objective scores between preselect using an ibf and no preselect.\n"
        << "Naive full histogram rebuild:  " << res1.objective << "\n"
        << "Memorization update objective: " << res2.objective << "\n";
    EXPECT_EQ(res1.iterations, res2.iterations) 
        << "Expected matching iterations between preselect using an ibf and no preselect.\n"
        << "Naive full histogram rebuild:  " << res1.iterations << "\n"
        << "Memorization update objective: " << res2.iterations << "\n";
        //EXPECT_EQ(true, false);
}