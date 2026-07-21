#include <gtest/gtest.h>

#include "matching_engine/book/PriceLevel.hpp"

using matching_engine::book::PriceLevel;
using matching_engine::core::Order;

class PriceLevelTest : public ::testing::Test
{
protected:
    PriceLevel level;

    Order order1{};
    Order order2{};
    Order order3{};
};

TEST_F(PriceLevelTest, InitiallyEmpty)
{
    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.size(), 0u);
    EXPECT_EQ(level.front(), nullptr);
    EXPECT_EQ(level.back(), nullptr);
}

TEST_F(PriceLevelTest, PushSingleOrder)
{
    level.push_back(&order1);

    EXPECT_FALSE(level.empty());
    EXPECT_EQ(level.size(), 1u);

    EXPECT_EQ(level.front(), &order1);
    EXPECT_EQ(level.back(), &order1);
}

TEST_F(PriceLevelTest, MaintainsFIFOOrder)
{
    level.push_back(&order1);
    level.push_back(&order2);
    level.push_back(&order3);

    EXPECT_EQ(level.front(), &order1);
    EXPECT_EQ(level.back(), &order3);

    EXPECT_EQ(order1.next, &order2);
    EXPECT_EQ(order2.next, &order3);

    EXPECT_EQ(order3.prev, &order2);
    EXPECT_EQ(order2.prev, &order1);
}

TEST_F(PriceLevelTest, RemoveHead)
{
    level.push_back(&order1);
    level.push_back(&order2);

    level.erase(&order1);

    EXPECT_EQ(level.front(), &order2);
    EXPECT_EQ(level.back(), &order2);
    EXPECT_EQ(level.size(), 1u);

    EXPECT_EQ(order1.prev, nullptr);
    EXPECT_EQ(order1.next, nullptr);
}

TEST_F(PriceLevelTest, RemoveTail)
{
    level.push_back(&order1);
    level.push_back(&order2);

    level.erase(&order2);

    EXPECT_EQ(level.front(), &order1);
    EXPECT_EQ(level.back(), &order1);
    EXPECT_EQ(level.size(), 1u);
}

TEST_F(PriceLevelTest, RemoveMiddle)
{
    level.push_back(&order1);
    level.push_back(&order2);
    level.push_back(&order3);

    level.erase(&order2);

    EXPECT_EQ(order1.next, &order3);
    EXPECT_EQ(order3.prev, &order1);

    EXPECT_EQ(level.size(), 2u);
}

TEST_F(PriceLevelTest, RemoveOnlyElement)
{
    level.push_back(&order1);

    level.erase(&order1);

    EXPECT_TRUE(level.empty());

    EXPECT_EQ(level.front(), nullptr);
    EXPECT_EQ(level.back(), nullptr);
}

TEST_F(PriceLevelTest, MultipleOperations)
{
    level.push_back(&order1);
    level.push_back(&order2);
    level.erase(&order1);

    level.push_back(&order3);

    EXPECT_EQ(level.front(), &order2);
    EXPECT_EQ(level.back(), &order3);

    EXPECT_EQ(level.size(), 2u);
}