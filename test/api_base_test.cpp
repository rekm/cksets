#include <gtest/gtest.h>

#include "app_test.hpp"
#include <ksets/ksets.hpp>
#include <vector>

struct test_api : public app_test
{};

TEST_F(test_api, simple_api_test){
    std::vector<std::vector<ksets::token_t>> data{{1,2,3,4,5}, {6,2,5,4}, {2,3,4,8}, {2,1,4,7}, {1,2,3,4}};
    
}