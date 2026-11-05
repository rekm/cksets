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

TEST_F(test_api, simple_api_test_histograms){
    std::vector<std::vector<ksets::token_t>> data{{1, 5, 2, 3, 6}, {1, 5, 2, 11, 12}};
    ksets::ClusterOptions opts{.clusters=2, .hist_type=ksets::CollectionType::mapRobin};
    auto hists = ksets::histograms(opts);
    EXPECT_EQ(hists->size(), 2) << "Expected histograms to be of size 2\n";
    std::vector<ksets::hist_count_t> hist1_values{5, 5, 4, 2, 1, 1, 1, 1, 1, 1};
    for (size_t i=0; i<hist1_values.size(); i++){
        hists->set(0,i+1,hist1_values[i]);
    }
    for (size_t i=0; i<hist1_values.size(); i++){
        EXPECT_EQ(hists->get(0,i+1), hist1_values[i]) << "Error for index " << i << " of " << hist1_values.size() << "!\n";
    }
    std::vector<ksets::hist_count_t> hist2_values{1, 1, 1, 1, 1, 3, 2, 4, 5, 3, 1};
    for (size_t i=0; i<hist2_values.size(); i++){
        hists->set(1,i+1,hist2_values[i]);
    }
    for (size_t i=0; i<hist2_values.size(); i++){
        EXPECT_EQ(hists->get(1,i+1), hist2_values[i]) << "Error for index " << i << " of " << hist2_values.size() << "!\n";
    }

    ksets::ClusterResult res1; 
    res1 = ksets::cluster(data,hists,opts);
    std::cerr << "Objective Score First Run: " << res1.objective << " Iterations: " << res1.iterations <<"\n";
}

TEST_F(test_api, simple_api_kswaps_test){
    std::vector<std::vector<ksets::token_t>> data{{1,2,3,4,5}, {6,2,5,4}, {2,3,4,8}, {2,1,4,7}, {1,2,3,4}};
    ksets::ClusterOptions opts{.clusters=2, .max_iterations=3};
    ksets::ClusterResult res1; 
    res1 = ksets::cluster_swaps(data,3,opts);
    std::cerr << "Objective Score First Run: " << res1.objective << " Iterations: " << res1.iterations <<"\n";
}