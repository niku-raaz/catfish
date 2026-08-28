#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

#define CATFISH_EXPECT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "EXPECT_TRUE failed: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; \
            std::exit(1); \
        } \
    } while (false)

#define CATFISH_EXPECT_EQ(actual, expected) \
    do { \
        const auto actual_value = (actual); \
        const auto expected_value = (expected); \
        if (!(actual_value == expected_value)) { \
            std::cerr << "EXPECT_EQ failed at " << __FILE__ << ':' << __LINE__ \
                      << " actual=" << actual_value << " expected=" << expected_value << '\n'; \
            std::exit(1); \
        } \
    } while (false)

