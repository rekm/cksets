// SPDX-FileCopyrightText: 2006-2025 Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2025 Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: CC0-1.0

#include <gtest/gtest.h>

#include "app_test.hpp"
#include <ksets/types.hpp>
#include <ksets/distance/jaccard.hpp>
#include <ksets/hashing/murmur_hash.hpp>
#include <ksets/histogram/collection_map.hpp>
#include <ksets/preselect/ibf_utils.hpp>

#include <iostream>

struct test_histogram : public app_test
{};

struct test_distance : public app_test
{};

struct test_distance_threshold : public app_test
{};

TEST_F(test_histogram, histogram_store_and__simple_retrieval){
    uint number_of_clusters{2};
    auto hash_policy = ksets::hashing::HashPolicy(ksets::hashing::MurmurHash(42));
    ksets::histogram::MapHistogramCollection hist_collection(number_of_clusters, hash_policy);
    ksets::distance::JaccardDistance dist(hist_collection);
    std::vector<unsigned> hist1_paper = {5, 5, 4, 2, 1, 1, 1, 1, 1, 1};
    for (size_t i = 0; i < hist1_paper.size(); ++i)
        hist_collection[0].set(i + 1, hist1_paper[i]);
    hist_collection[0].debug_print();
    for (const auto& kv : hist_collection[0].expose_map()) {
        EXPECT_EQ(hist_collection[0].get(kv.first), kv.second);
    }      

}


TEST_F(test_histogram, histogram_store_and__hash_retrieval){
    uint number_of_clusters{2};
    auto hash_policy = ksets::hashing::HashPolicy(ksets::hashing::MurmurHash(42));
    ksets::histogram::MapHistogramCollection hist_collection(number_of_clusters, hash_policy);
    ksets::distance::JaccardDistance dist(hist_collection);
    std::vector<unsigned> hist1_paper = {5, 5, 4, 2, 1, 1, 1, 1, 1, 1};
    for (size_t i = 0; i < hist1_paper.size(); ++i)
        hist_collection[0].set(i + 1, hist1_paper[i]);
    hist_collection[0].debug_print();
    size_t it = 0;   
    for (const auto& kv : hist_collection[0].expose_map()) {
        size_t hash = hash_policy(kv.first);
        EXPECT_EQ(hist_collection[0].get_with_hash(kv.first, hash), kv.second)
         << "While looking up value " << it+1 << " of " << hist1_paper.size() << "\n"
         << "hash(" << kv.first << ") = " << hash << "\n"
         << "pointed to a value of " <<  hist_collection[0].get_with_hash(kv.first,hash) << " instead of " << kv.second ;
         it++;
    }      

}

TEST_F(test_histogram, minimums_and_maximums){
    uint number_of_clusters{2};
    auto hash_policy = ksets::hashing::HashPolicy(ksets::hashing::MurmurHash(42));
    ksets::histogram::MapHistogramCollection hist_collection(number_of_clusters, hash_policy);
    ksets::distance::JaccardDistance dist(hist_collection);
    std::vector<unsigned> hist1_paper = {5, 5, 4, 2, 1, 1, 1, 1, 1, 1};
    for (size_t i = 0; i < hist1_paper.size(); ++i)
        hist_collection[0].set(i + 1, hist1_paper[i]);
    hist_collection[0].debug_print();
    std::vector<unsigned> hist2_paper = {1, 1, 1, 1, 1, 3, 2, 4, 5, 3, 1};
    for (size_t i = 0; i < hist2_paper.size(); ++i)
        hist_collection[1].set(i + 1, hist2_paper[i]);
    hist_collection[1].debug_print();
    auto  mins = hist_collection.minimums();
    EXPECT_EQ(mins[0], 1);
    EXPECT_EQ(mins[1], 1);
    auto  maxs = hist_collection.maximums();
    EXPECT_EQ(maxs[0], 5);
    EXPECT_EQ(maxs[1], 5);

}



TEST_F(test_distance, paper_test_slow_loop)
{
    uint number_of_clusters{2};
    auto hash_policy = ksets::hashing::HashPolicy(ksets::hashing::MurmurHash(42));
    ksets::histogram::MapHistogramCollection hist_collection(number_of_clusters, hash_policy);
    ksets::distance::JaccardDistance dist(hist_collection);
    std::vector<unsigned> hist1_paper = {5, 5, 4, 2, 1, 1, 1, 1, 1, 1};
    for (size_t i = 0; i < hist1_paper.size(); ++i)
        hist_collection[0].set(i + 1, hist1_paper[i]);
    hist_collection[0].debug_print();
    std::vector<unsigned> hist2_paper = {1, 1, 1, 1, 1, 3, 2, 4, 5, 3, 1};
    for (size_t i = 0; i < hist2_paper.size(); ++i)
        hist_collection[1].set(i + 1, hist2_paper[i]);
    hist_collection[1].debug_print();
    std::vector<size_t> range{0, 1}; // If this is not size_t, get_range seg_faults.... ToDo: Investigate
    auto full_range = hist_collection.get_range(range);
    std::vector<unsigned> row1 = {1, 5, 2, 3, 6}; // A, E, B, C, F
    EXPECT_EQ(full_range.size(), 2);
    dist.begin_iteration(2);
    dist.begin_row();
    for (auto t : row1)
        dist.accumulate_token(t, full_range);
    auto const & d = dist.finalize();
    EXPECT_NEAR(d[0], 0.2727, 1e-4);
    EXPECT_NEAR(d[1], 0.6956, 1e-4);
    std::vector<unsigned> row2 = {1, 5, 2, 11, 12}; // A, E, B, K, L
    dist.begin_row();
    for (auto t : row2)
        dist.accumulate_token(t, full_range);
    auto const & d2 = dist.finalize();
    EXPECT_NEAR(d2[0], 0.5416, 1e-4); // In the paper the example misses the subtraction by 1
    EXPECT_NEAR(d2[1], 0.8333, 1e-4);
}

TEST_F(test_distance, paper_test_fast_loop)
{
    uint number_of_clusters{2};
    auto hash_policy = ksets::hashing::HashPolicy(ksets::hashing::MurmurHash(42));
    ksets::histogram::MapHistogramCollection hist_collection(number_of_clusters, hash_policy);
    ksets::distance::JaccardDistance dist(hist_collection);
    std::vector<ksets::token_t> hist1_paper = {5, 5, 4, 2, 1, 1, 1, 1, 1, 1};
    for (size_t i = 0; i < hist1_paper.size(); ++i)
        hist_collection[0].set(i + 1, hist1_paper[i]);
    hist_collection[0].debug_print();
    std::vector<ksets::token_t> hist2_paper = {1, 1, 1, 1, 1, 3, 2, 4, 5, 3, 1};
    for (size_t i = 0; i < hist2_paper.size(); ++i)
        hist_collection[1].set(i + 1, hist2_paper[i]);
    hist_collection[1].debug_print();
    std::vector<size_t> range{0, 1}; // If this is not size_t, get_range seg_faults.... ToDo: Investigate
    auto full_range = hist_collection.get_range(range);
    std::vector<ksets::token_t> row1 = {1, 5, 2, 3, 6}; // A, E, B, C, F
    EXPECT_EQ(full_range.size(), 2);
    dist.begin_iteration(2);
    dist.begin_row();
    dist.accumulate_token(row1, full_range);
    auto const & d = dist.finalize();
    EXPECT_NEAR(d[0], 0.2727, 1e-4);
    EXPECT_NEAR(d[1], 0.6956, 1e-4);
    std::vector<ksets::token_t> row2 = {1, 5, 2, 11, 12}; // A, E, B, K, L
    dist.begin_row();
    dist.accumulate_token(row2, full_range);
    auto const & d2 = dist.finalize();
    EXPECT_NEAR(d2[0], 0.5416, 1e-4); // In the paper the example misses the subtraction by 1
    EXPECT_NEAR(d2[1], 0.8333, 1e-4);
}


TEST_F(test_distance, paper_test_singular_get)
{
    uint number_of_clusters{2};
    auto hash_policy = ksets::hashing::HashPolicy(ksets::hashing::MurmurHash(42));
    ksets::histogram::MapHistogramCollection hist_collection(number_of_clusters, hash_policy);
    ksets::distance::JaccardDistance dist(hist_collection);
    std::vector<ksets::token_t> hist1_paper = {5, 5, 4, 2, 1, 1, 1, 1, 1, 1};
    for (size_t i = 0; i < hist1_paper.size(); ++i)
        hist_collection[0].set(i + 1, hist1_paper[i]);
    hist_collection[0].debug_print();
    std::vector<ksets::token_t> hist2_paper = {1, 1, 1, 1, 1, 3, 2, 4, 5, 3, 1};
    for (size_t i = 0; i < hist2_paper.size(); ++i)
        hist_collection[1].set(i + 1, hist2_paper[i]);
    hist_collection[1].debug_print();
    std::vector<ksets::token_t> row1 = {1, 5, 2, 3, 6}; // A, E, B, C, F
    dist.begin_iteration(0);
    
    EXPECT_NEAR(dist.get(row1, 0), 0.2727, 1e-4);
    EXPECT_NEAR(dist.get(row1, 1), 0.6956, 1e-4);
    std::vector<ksets::token_t> row2 = {1, 5, 2, 11, 12}; // A, E, B, K, L
    EXPECT_NEAR(dist.get(row2, 0), 0.5416, 1e-4); // In the paper the example misses the subtraction by 1
    EXPECT_NEAR(dist.get(row2, 1), 0.8333, 1e-4);
}


TEST_F(test_distance_threshold, paper_test_threshold_calc)
{
    uint number_of_clusters{3};
    auto hash_policy = ksets::hashing::HashPolicy(ksets::hashing::MurmurHash(42));
    ksets::histogram::MapHistogramCollection hist_collection(number_of_clusters, hash_policy);
    ksets::distance::JaccardDistance dist(hist_collection);
    std::vector<ksets::token_t> hist1_paper = {5, 5, 4, 2, 1, 1, 1, 1, 1, 1};
    for (size_t i = 0; i < hist1_paper.size(); ++i)
        hist_collection[0].set(i + 1, hist1_paper[i]);
    hist_collection[0].debug_print();
    std::vector<ksets::token_t> hist2_paper = {1, 1, 1, 1, 1, 3, 2, 4, 5, 3, 1};
    for (size_t i = 0; i < hist2_paper.size(); ++i)
        hist_collection[1].set(i + 1, hist2_paper[i]);
    hist_collection[1].debug_print();
    std::vector<std::vector<ksets::token_t>> data = {
        {1, 5, 2, 3, 6}, 
        {1, 5, 2, 11, 12}
    };
    
    ksets::preselect::JaccardThresholdPolicy threshold(hist_collection, dist);
    dist.begin_iteration(0);
    std::vector<ksets::label_t> labels_cur = {1,1};
    std::vector<double> precomputed_distances = {1.0, 1.0};
    for (size_t i=0; i<data.size(); i++ ){
        precomputed_distances[i] = dist.get(data[i], labels_cur[i]);
    }
    threshold.before_iteration(0);
    
    for (size_t i=0; i<data.size(); i++){
        auto const& thresholds = threshold.compute_min_thresholds(precomputed_distances[i], data[i].size());
        std::cerr << "Min Threshold Values for row" << i << ":"; 
        for (auto val: thresholds){
            std::cerr << " " << val; 
        }
        std::cerr << "\n";
    }

    for (size_t i=0; i<data.size(); i++){
        auto const& thresholds = threshold.compute_max_thresholds(precomputed_distances[i], data[i].size());
        std::cerr << "Max Threshold Values for row " << i << ":"; 
        for (auto val: thresholds){
            std::cerr << " " << val; 
        }
        std::cerr << "\n";
    }
    
    
    EXPECT_EQ(true, true);
     // A, E, B, C, F
    //EXPECT_NEAR(dist.get(data[0], 0), 0.2727, 1e-4);
    //EXPECT_NEAR(dist.get(data[0], 1), 0.6956, 1e-4);
    // A, E, B, K, L
    //EXPECT_NEAR(dist.get(data[1], 0), 0.5416, 1e-4); // In the paper the example misses the subtraction by 1
    //EXPECT_NEAR(dist.get(data[1], 1), 0.8333, 1e-4);
}