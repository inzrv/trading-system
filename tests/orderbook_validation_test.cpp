#include <gtest/gtest.h>

#include "common/orderbook_validation.h"

namespace
{

TEST(OrderbookValidationTest, AcceptsEmptyL1)
{
    const L1 l1{};

    EXPECT_TRUE(is_valid(l1));
}

TEST(OrderbookValidationTest, AcceptsSingleSidedBook)
{
    const L1 l1{
        .best_bid = Level{.price = 100.0, .quantity = 1.0},
        .best_ask = std::nullopt
    };

    EXPECT_TRUE(is_valid(l1));
}

TEST(OrderbookValidationTest, AcceptsNonCrossedBook)
{
    const L1 l1{
        .best_bid = Level{.price = 100.0, .quantity = 1.0},
        .best_ask = Level{.price = 100.5, .quantity = 2.0}
    };

    EXPECT_TRUE(is_valid(l1));
}

TEST(OrderbookValidationTest, AcceptsLockedBook)
{
    const L1 l1{
        .best_bid = Level{.price = 100.0, .quantity = 1.0},
        .best_ask = Level{.price = 100.0, .quantity = 2.0}
    };

    EXPECT_TRUE(is_valid(l1));
}

TEST(OrderbookValidationTest, RejectsCrossedBook)
{
    const L1 l1{
        .best_bid = Level{.price = 100.5, .quantity = 1.0},
        .best_ask = Level{.price = 100.0, .quantity = 2.0}
    };

    EXPECT_FALSE(is_valid(l1));
}

} // namespace
