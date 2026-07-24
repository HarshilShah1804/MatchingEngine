#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "matching_engine/book/OrderBook.hpp"

using matching_engine::book::OrderBook;
using matching_engine::core::Order;
using matching_engine::core::OrderType;
using matching_engine::core::Side;

class OrderBookTest : public ::testing::Test
{
protected:
    OrderBook book;

    Order create_order(matching_engine::core::OrderID id,
                       matching_engine::core::Price price,
                       matching_engine::core::Quantity quantity,
                       Side side)
    {
        return Order{
            .id = id,
            .symbol = 1,
            .side = side,
            .type = OrderType::Limit,
            .price = price,
            .initial_quantity = quantity,
            .remaining_quantity = quantity,
            .timestamp = 0,
        };
    }
};

// ============================================================================
// Basic Insert Tests
// ============================================================================

TEST_F(OrderBookTest, InitiallyEmpty)
{
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.size(), 0u);
    EXPECT_EQ(book.best_bid_order(), nullptr);
    EXPECT_EQ(book.best_ask_order(), nullptr);
}

TEST_F(OrderBookTest, InsertSingleBuyOrder)
{
    Order buy = create_order(1, 100, 10, Side::Buy);
    bool inserted = book.insert(&buy);

    EXPECT_TRUE(inserted);
    EXPECT_FALSE(book.empty());
    EXPECT_EQ(book.size(), 1u);
    EXPECT_EQ(book.best_bid_order()->id, 1u);
}

TEST_F(OrderBookTest, InsertSingleSellOrder)
{
    Order sell = create_order(1, 100, 10, Side::Sell);
    bool inserted = book.insert(&sell);

    EXPECT_TRUE(inserted);
    EXPECT_FALSE(book.empty());
    EXPECT_EQ(book.size(), 1u);
    EXPECT_EQ(book.best_ask_order()->id, 1u);
}

TEST_F(OrderBookTest, InsertMultipleBuyOrders)
{
    Order buy1 = create_order(1, 100, 10, Side::Buy);
    Order buy2 = create_order(2, 100, 10, Side::Buy);
    Order buy3 = create_order(3, 100, 10, Side::Buy);

    book.insert(&buy1);
    book.insert(&buy2);
    book.insert(&buy3);

    EXPECT_EQ(book.size(), 3u);
    EXPECT_EQ(book.best_bid_order()->id, 1u);
}

TEST_F(OrderBookTest, InsertMultipleSellOrders)
{
    Order sell1 = create_order(1, 100, 10, Side::Sell);
    Order sell2 = create_order(2, 100, 10, Side::Sell);
    Order sell3 = create_order(3, 100, 10, Side::Sell);

    book.insert(&sell1);
    book.insert(&sell2);
    book.insert(&sell3);

    EXPECT_EQ(book.size(), 3u);
    EXPECT_EQ(book.best_ask_order()->id, 1u);
}

// ============================================================================
// Price Priority Tests
// ============================================================================

TEST_F(OrderBookTest, BestBidIsPriceHighest)
{
    Order buy1 = create_order(1, 95, 10, Side::Buy);
    Order buy2 = create_order(2, 100, 10, Side::Buy);
    Order buy3 = create_order(3, 98, 10, Side::Buy);

    book.insert(&buy1);
    book.insert(&buy2);
    book.insert(&buy3);

    EXPECT_EQ(book.best_bid_order()->id, 2u);
    EXPECT_EQ(book.best_bid_order()->price, 100);
}

TEST_F(OrderBookTest, BestAskIsLowestPrice)
{
    Order sell1 = create_order(1, 105, 10, Side::Sell);
    Order sell2 = create_order(2, 100, 10, Side::Sell);
    Order sell3 = create_order(3, 102, 10, Side::Sell);

    book.insert(&sell1);
    book.insert(&sell2);
    book.insert(&sell3);

    EXPECT_EQ(book.best_ask_order()->id, 2u);
    EXPECT_EQ(book.best_ask_order()->price, 100);
}

// ============================================================================
// Best Price Query Tests
// ============================================================================

TEST_F(OrderBookTest, BestBidPriceReturnsCorrectPrice)
{
    Order buy = create_order(1, 100, 10, Side::Buy);
    book.insert(&buy);

    auto price = book.best_bid_price();
    EXPECT_TRUE(price.has_value());
    EXPECT_EQ(price.value(), 100);
}

TEST_F(OrderBookTest, BestAskPriceReturnsCorrectPrice)
{
    Order sell = create_order(1, 100, 10, Side::Sell);
    book.insert(&sell);

    auto price = book.best_ask_price();
    EXPECT_TRUE(price.has_value());
    EXPECT_EQ(price.value(), 100);
}

TEST_F(OrderBookTest, BestBidPriceNoneWhenEmpty)
{
    auto price = book.best_bid_price();
    EXPECT_FALSE(price.has_value());
}

TEST_F(OrderBookTest, BestAskPriceNoneWhenEmpty)
{
    auto price = book.best_ask_price();
    EXPECT_FALSE(price.has_value());
}

// ============================================================================
// Cancel Tests
// ============================================================================

TEST_F(OrderBookTest, CancelSingleOrder)
{
    Order buy = create_order(1, 100, 10, Side::Buy);
    book.insert(&buy);

    bool cancelled = book.cancel(1);

    EXPECT_TRUE(cancelled);
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.size(), 0u);
}

TEST_F(OrderBookTest, CancelOneOfMultipleAtSamePrice)
{
    Order buy1 = create_order(1, 100, 10, Side::Buy);
    Order buy2 = create_order(2, 100, 10, Side::Buy);
    Order buy3 = create_order(3, 100, 10, Side::Buy);

    book.insert(&buy1);
    book.insert(&buy2);
    book.insert(&buy3);

    bool cancelled = book.cancel(2);

    EXPECT_TRUE(cancelled);
    EXPECT_EQ(book.size(), 2u);
    EXPECT_EQ(book.best_bid_order()->id, 1u);
}

TEST_F(OrderBookTest, CancelNonexistentOrderReturnsFalse)
{
    bool cancelled = book.cancel(999);

    EXPECT_FALSE(cancelled);
    EXPECT_TRUE(book.empty());
}

TEST_F(OrderBookTest, CancelRemovesEmptyPriceLevel)
{
    Order buy = create_order(1, 100, 10, Side::Buy);
    book.insert(&buy);

    book.cancel(1);

    EXPECT_EQ(book.best_bid_price(), std::nullopt);
}

TEST_F(OrderBookTest, CancelLeavesOtherPriceLevels)
{
    Order buy1 = create_order(1, 100, 10, Side::Buy);
    Order buy2 = create_order(2, 95, 10, Side::Buy);

    book.insert(&buy1);
    book.insert(&buy2);

    book.cancel(1);

    EXPECT_EQ(book.size(), 1u);
    EXPECT_EQ(book.best_bid_order()->id, 2u);
}

// ============================================================================
// Mixed Buy/Sell Tests
// ============================================================================

TEST_F(OrderBookTest, MixedBidAndAskOrders)
{
    Order buy1 = create_order(1, 100, 10, Side::Buy);
    Order sell1 = create_order(2, 105, 10, Side::Sell);
    Order buy2 = create_order(3, 99, 10, Side::Buy);
    Order sell2 = create_order(4, 106, 10, Side::Sell);

    book.insert(&buy1);
    book.insert(&sell1);
    book.insert(&buy2);
    book.insert(&sell2);

    EXPECT_EQ(book.size(), 4u);
    EXPECT_EQ(book.best_bid_order()->id, 1u);
    EXPECT_EQ(book.best_ask_order()->id, 2u);
}

TEST_F(OrderBookTest, BidAndAskIndependent)
{
    Order buy = create_order(1, 100, 10, Side::Buy);
    Order sell = create_order(2, 105, 10, Side::Sell);

    book.insert(&buy);
    book.insert(&sell);

    book.cancel(1);

    EXPECT_EQ(book.size(), 1u);
    EXPECT_EQ(book.best_ask_order()->id, 2u);
    EXPECT_EQ(book.best_bid_order(), nullptr);
}

// ============================================================================
// Multiple Price Level Tests
// ============================================================================

TEST_F(OrderBookTest, MultipleBidLevelsOrderedByPrice)
{
    Order buy100 = create_order(1, 100, 5, Side::Buy);
    Order buy95 = create_order(2, 95, 5, Side::Buy);
    Order buy102 = create_order(3, 102, 5, Side::Buy);
    Order buy98 = create_order(4, 98, 5, Side::Buy);

    book.insert(&buy100);
    book.insert(&buy95);
    book.insert(&buy102);
    book.insert(&buy98);

    // Best bid should be 102
    EXPECT_EQ(book.best_bid_order()->id, 3u);
}

TEST_F(OrderBookTest, MultipleAskLevelsOrderedByPrice)
{
    Order sell100 = create_order(1, 100, 5, Side::Sell);
    Order sell105 = create_order(2, 105, 5, Side::Sell);
    Order sell98 = create_order(3, 98, 5, Side::Sell);
    Order sell102 = create_order(4, 102, 5, Side::Sell);

    book.insert(&sell100);
    book.insert(&sell105);
    book.insert(&sell98);
    book.insert(&sell102);

    // Best ask should be 98
    EXPECT_EQ(book.best_ask_order()->id, 3u);
}

// ============================================================================
// Duplicate Order ID Tests
// ============================================================================

TEST_F(OrderBookTest, DuplicateOrderIDNotInserted)
{
    Order buy1 = create_order(1, 100, 10, Side::Buy);
    Order buy2 = create_order(1, 105, 10, Side::Buy);

    bool inserted1 = book.insert(&buy1);
    bool inserted2 = book.insert(&buy2);

    EXPECT_TRUE(inserted1);
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(book.size(), 1u);
}

// ============================================================================
// Market Order Tests
// ============================================================================

TEST_F(OrderBookTest, MarketOrderNotInserted)
{
    Order market = create_order(1, 100, 10, Side::Buy);
    market.type = OrderType::Market;

    bool inserted = book.insert(&market);

    EXPECT_FALSE(inserted);
    EXPECT_TRUE(book.empty());
}

// ============================================================================
// Order Cancellation Sequence Tests
// ============================================================================

TEST_F(OrderBookTest, CancelFirstOrderAtLevel)
{
    Order buy1 = create_order(1, 100, 10, Side::Buy);
    Order buy2 = create_order(2, 100, 10, Side::Buy);
    Order buy3 = create_order(3, 100, 10, Side::Buy);

    book.insert(&buy1);
    book.insert(&buy2);
    book.insert(&buy3);

    // Cancel first (FIFO order)
    book.cancel(1);

    EXPECT_EQ(book.size(), 2u);
    EXPECT_EQ(book.best_bid_order()->id, 2u);
}

TEST_F(OrderBookTest, CancelMiddleOrderAtLevel)
{
    Order buy1 = create_order(1, 100, 10, Side::Buy);
    Order buy2 = create_order(2, 100, 10, Side::Buy);
    Order buy3 = create_order(3, 100, 10, Side::Buy);

    book.insert(&buy1);
    book.insert(&buy2);
    book.insert(&buy3);

    book.cancel(2);

    EXPECT_EQ(book.size(), 2u);
    EXPECT_EQ(book.best_bid_order()->id, 1u);
}

TEST_F(OrderBookTest, CancelLastOrderAtLevel)
{
    Order buy1 = create_order(1, 100, 10, Side::Buy);
    Order buy2 = create_order(2, 100, 10, Side::Buy);
    Order buy3 = create_order(3, 100, 10, Side::Buy);

    book.insert(&buy1);
    book.insert(&buy2);
    book.insert(&buy3);

    book.cancel(3);

    EXPECT_EQ(book.size(), 2u);
    EXPECT_EQ(book.best_bid_order()->id, 1u);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(OrderBookTest, ManyOrdersAtDifferentPrices)
{
    const int NUM_ORDERS = 100;

    // Insert 100 buy orders at different prices
    std::vector<std::unique_ptr<Order>> orders;
    for (int i = 0; i < NUM_ORDERS; ++i)
    {
        auto order = std::make_unique<Order>(create_order(i, i + 1, 10, Side::Buy));
        book.insert(order.get());
        orders.push_back(std::move(order));
    }

    EXPECT_EQ(book.size(), NUM_ORDERS);
    EXPECT_EQ(book.best_bid_order()->price, NUM_ORDERS);
}

TEST_F(OrderBookTest, ManyOrdersAtSamePrice)
{
    const int NUM_ORDERS = 100;

    // Insert 100 buy orders at same price
    std::vector<std::unique_ptr<Order>> orders;
    for (int i = 0; i < NUM_ORDERS; ++i)
    {
        auto order = std::make_unique<Order>(create_order(i, 100, 10, Side::Buy));
        book.insert(order.get());
        orders.push_back(std::move(order));
    }

    EXPECT_EQ(book.size(), NUM_ORDERS);
    EXPECT_EQ(book.best_bid_order()->id, 0u);
}

TEST_F(OrderBookTest, SequentialInsertCancel)
{
    // Insert 10, cancel 5, insert 10 more
    std::vector<std::unique_ptr<Order>> orders;
    
    for (int i = 0; i < 10; ++i)
    {
        auto order = std::make_unique<Order>(create_order(i, 100, 10, Side::Buy));
        book.insert(order.get());
        orders.push_back(std::move(order));
    }

    for (int i = 0; i < 5; ++i)
    {
        book.cancel(i);
    }

    EXPECT_EQ(book.size(), 5u);

    for (int i = 10; i < 20; ++i)
    {
        auto order = std::make_unique<Order>(create_order(i, 100, 10, Side::Buy));
        book.insert(order.get());
        orders.push_back(std::move(order));
    }

    EXPECT_EQ(book.size(), 15u);
}
