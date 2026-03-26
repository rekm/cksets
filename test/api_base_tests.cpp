// SPDX-FileCopyrightText: 2006-2025 Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2025 Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: CC0-1.0

#include <gtest/gtest.h>

#include "app_test.hpp"
#include <ksets/types.hpp>
#include <ksets/distance/jaccard.hpp>
#include <ksets/hashing/murmur_hash.hpp>
#include <ksets/histogram/collection_map.hpp>

struct test_histogram : public app_test
{};

TEST_F(test_histogram, paper_test_slow_loop)
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

TEST_F(test_histogram, paper_test_fast_loop)
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
