#include <gtest/gtest.h>

#include "matching_engine/engine/MatchingEngine.hpp"

using matching_engine::engine::MatchingEngine;
using matching_engine::core::Order;
using matching_engine::core::OrderType;
using matching_engine::core::Side;
using matching_engine::core::Trade;

class MatchingEngineTest : public ::testing::Test
{
protected:
    MatchingEngine engine;

    Order create_order(matching_engine::core::OrderID id,
                       matching_engine::core::Price price,
                       matching_engine::core::Quantity quantity,
                       Side side,
                       OrderType type = OrderType::Limit)
    {
        return Order{
            .id = id,
            .symbol = 1,
            .side = side,
            .type = type,
            .price = price,
            .initial_quantity = quantity,
            .remaining_quantity = quantity,
            .timestamp = 0,
        };
    }
};

// ============================================================================
// Basic Matching Tests
// ============================================================================

TEST_F(MatchingEngineTest, NoMatchingWhenBookEmpty)
{
    Order buy_order = create_order(1, 100, 10, Side::Buy);
    auto trades = engine.submit(&buy_order);

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(buy_order.remaining_quantity, 10u);
    EXPECT_EQ(engine.order_book().size(), 1u);
}

TEST_F(MatchingEngineTest, SimpleMatchBuyWithSell)
{
    // Insert sell order first
    Order sell_order = create_order(1, 100, 10, Side::Sell);
    engine.submit(&sell_order);

    // Submit matching buy order
    Order buy_order = create_order(2, 100, 10, Side::Buy);
    auto trades = engine.submit(&buy_order);

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].buy_order, 2u);
    EXPECT_EQ(trades[0].sell_order, 1u);
    EXPECT_EQ(trades[0].execution_price, 100);
    EXPECT_EQ(trades[0].quantity, 10u);

    EXPECT_EQ(buy_order.remaining_quantity, 0u);
    EXPECT_EQ(sell_order.remaining_quantity, 0u);
    EXPECT_TRUE(engine.order_book().empty());
}

TEST_F(MatchingEngineTest, SimpleMatchSellWithBuy)
{
    // Insert buy order first
    Order buy_order = create_order(1, 100, 10, Side::Buy);
    engine.submit(&buy_order);

    // Submit matching sell order
    Order sell_order = create_order(2, 100, 10, Side::Sell);
    auto trades = engine.submit(&sell_order);

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].buy_order, 1u);
    EXPECT_EQ(trades[0].sell_order, 2u);
    EXPECT_EQ(trades[0].execution_price, 100);
    EXPECT_EQ(trades[0].quantity, 10u);

    EXPECT_EQ(buy_order.remaining_quantity, 0u);
    EXPECT_EQ(sell_order.remaining_quantity, 0u);
    EXPECT_TRUE(engine.order_book().empty());
}

// ============================================================================
// Partial Fill Tests
// ============================================================================

TEST_F(MatchingEngineTest, BuyOrderPartiallyFillsMultipleSellOrders)
{
    // Insert multiple sell orders
    Order sell1 = create_order(1, 100, 5, Side::Sell);
    engine.submit(&sell1);

    Order sell2 = create_order(2, 100, 5, Side::Sell);
    engine.submit(&sell2);

    // Submit larger buy order
    Order buy = create_order(3, 100, 12, Side::Buy);
    auto trades = engine.submit(&buy);

    EXPECT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].quantity, 5u);
    EXPECT_EQ(trades[1].quantity, 5u);
    EXPECT_EQ(buy.remaining_quantity, 2u);
    EXPECT_EQ(sell1.remaining_quantity, 0u);
    EXPECT_EQ(sell2.remaining_quantity, 0u);

    EXPECT_EQ(engine.order_book().size(), 1u);
}

TEST_F(MatchingEngineTest, BuyOrderPartiallyFilledRemainsInBook)
{
    // Insert sell order
    Order sell = create_order(1, 100, 5, Side::Sell);
    engine.submit(&sell);

    // Submit larger buy order
    Order buy = create_order(2, 100, 10, Side::Buy);
    auto trades = engine.submit(&buy);

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 5u);
    EXPECT_EQ(buy.remaining_quantity, 5u);

    EXPECT_EQ(engine.order_book().size(), 1u);
    EXPECT_EQ(engine.order_book().best_bid_order()->id, 2u);
}

// ============================================================================
// Price Priority Tests
// ============================================================================

TEST_F(MatchingEngineTest, BuyOrderCrossesBestAskPrice)
{
    // Insert sell orders at different prices
    Order sell_high = create_order(1, 105, 10, Side::Sell);
    engine.submit(&sell_high);

    Order sell_best = create_order(2, 100, 10, Side::Sell);
    engine.submit(&sell_best);

    // Buy at 102 should match with sell at 100 (best ask)
    Order buy = create_order(3, 102, 10, Side::Buy);
    auto trades = engine.submit(&buy);

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].sell_order, 2u);
    EXPECT_EQ(trades[0].execution_price, 100);
}

TEST_F(MatchingEngineTest, SellOrderCrossesBestBidPrice)
{
    // Insert buy orders at different prices
    Order buy_low = create_order(1, 95, 10, Side::Buy);
    engine.submit(&buy_low);

    Order buy_best = create_order(2, 100, 10, Side::Buy);
    engine.submit(&buy_best);

    // Sell at 98 should match with buy at 100 (best bid)
    Order sell = create_order(3, 98, 10, Side::Sell);
    auto trades = engine.submit(&sell);

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].buy_order, 2u);
    EXPECT_EQ(trades[0].execution_price, 100);
}

// ============================================================================
// No Match Scenarios
// ============================================================================

TEST_F(MatchingEngineTest, BuyOrderDoesNotCrossSellOrder)
{
    // Insert sell order at 105
    Order sell = create_order(1, 105, 10, Side::Sell);
    engine.submit(&sell);

    // Buy order at 100 should not cross
    Order buy = create_order(2, 100, 10, Side::Buy);
    auto trades = engine.submit(&buy);

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(buy.remaining_quantity, 10u);
    EXPECT_EQ(engine.order_book().size(), 2u);
}

TEST_F(MatchingEngineTest, SellOrderDoesNotCrossBuyOrder)
{
    // Insert buy order at 95
    Order buy = create_order(1, 95, 10, Side::Buy);
    engine.submit(&buy);

    // Sell order at 100 should not cross
    Order sell = create_order(2, 100, 10, Side::Sell);
    auto trades = engine.submit(&sell);

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(sell.remaining_quantity, 10u);
    EXPECT_EQ(engine.order_book().size(), 2u);
}

// ============================================================================
// Time Priority Tests (FIFO within same price level)
// ============================================================================

TEST_F(MatchingEngineTest, MatchesFirstInFirstOutAtSamePrice)
{
    // Insert two sell orders at same price
    Order sell1 = create_order(1, 100, 10, Side::Sell);
    engine.submit(&sell1);

    Order sell2 = create_order(2, 100, 10, Side::Sell);
    engine.submit(&sell2);

    // Buy order should match with sell1 (FIFO)
    Order buy = create_order(3, 100, 15, Side::Buy);
    auto trades = engine.submit(&buy);

    EXPECT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].sell_order, 1u);
    EXPECT_EQ(trades[0].quantity, 10u);
    EXPECT_EQ(trades[1].sell_order, 2u);
    EXPECT_EQ(trades[1].quantity, 5u);
}

// ============================================================================
// Market Order Tests (Not Supported)
// ============================================================================

TEST_F(MatchingEngineTest, MarketOrderReturnsNoTrades)
{
    // Insert sell order
    Order sell = create_order(1, 100, 10, Side::Sell);
    engine.submit(&sell);

    // Market buy order should be rejected
    Order market_buy = create_order(2, 0, 10, Side::Buy, OrderType::Market);
    auto trades = engine.submit(&market_buy);

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(engine.order_book().size(), 1u);
}

// ============================================================================
// Complex Scenarios
// ============================================================================

TEST_F(MatchingEngineTest, MultipleLevelsWithPartialFills)
{
    // Build order book: sells at 100, 101, 102
    Order sell100 = create_order(1, 100, 5, Side::Sell);
    engine.submit(&sell100);

    Order sell101 = create_order(2, 101, 5, Side::Sell);
    engine.submit(&sell101);

    Order sell102 = create_order(3, 102, 5, Side::Sell);
    engine.submit(&sell102);

    // Aggressive buy should cross multiple levels
    Order buy = create_order(4, 102, 12, Side::Buy);
    auto trades = engine.submit(&buy);

    EXPECT_EQ(trades.size(), 3u);
    EXPECT_EQ(trades[0].execution_price, 100);
    EXPECT_EQ(trades[0].quantity, 5u);
    EXPECT_EQ(trades[1].execution_price, 101);
    EXPECT_EQ(trades[1].quantity, 5u);
    EXPECT_EQ(trades[2].execution_price, 102);
    EXPECT_EQ(trades[2].quantity, 2u);

    EXPECT_EQ(buy.remaining_quantity, 0u);
    EXPECT_EQ(engine.order_book().size(), 1u);
}

TEST_F(MatchingEngineTest, BuySellBuyCycle)
{
    // Insert initial sell order
    Order sell = create_order(1, 100, 10, Side::Sell);
    auto trades0 = engine.submit(&sell);
    EXPECT_EQ(trades0.size(), 0u);

    // Buy partially
    Order buy1 = create_order(2, 100, 5, Side::Buy);
    auto trades1 = engine.submit(&buy1);
    EXPECT_EQ(trades1.size(), 1u);

    // Insert another sell
    Order sell2 = create_order(3, 101, 10, Side::Sell);
    auto trades_sell2 = engine.submit(&sell2);
    EXPECT_EQ(trades_sell2.size(), 0u);

    // Buy again
    Order buy2 = create_order(4, 101, 8, Side::Buy);
    auto trades2 = engine.submit(&buy2);
    EXPECT_EQ(trades2.size(), 2u);
    EXPECT_EQ(trades2[0].sell_order, 1u);
    EXPECT_EQ(trades2[0].quantity, 5u);
    EXPECT_EQ(trades2[1].sell_order, 3u);
    EXPECT_EQ(trades2[1].quantity, 3u);
}

// ============================================================================
// Order Book State Tests
// ============================================================================

TEST_F(MatchingEngineTest, UnmatchedOrdersAccumulateInBook)
{
    // Add several unmatched buy orders
    Order buy1 = create_order(1, 90, 5, Side::Buy);
    engine.submit(&buy1);

    Order buy2 = create_order(2, 95, 5, Side::Buy);
    engine.submit(&buy2);

    Order buy3 = create_order(3, 100, 5, Side::Buy);
    engine.submit(&buy3);

    EXPECT_EQ(engine.order_book().size(), 3u);
    EXPECT_EQ(engine.order_book().best_bid_order()->id, 3u);
}

TEST_F(MatchingEngineTest, FullyFilledOrdersNotInBook)
{
    Order sell = create_order(1, 100, 5, Side::Sell);
    auto trades_sell = engine.submit(&sell);
    EXPECT_EQ(trades_sell.size(), 0u);

    Order buy = create_order(2, 100, 5, Side::Buy);
    auto trades_buy = engine.submit(&buy);
    EXPECT_EQ(trades_buy.size(), 1u);

    // Both orders fully filled, book should be empty
    EXPECT_TRUE(engine.order_book().empty());
}

TEST_F(MatchingEngineTest, PartiallyFilledOrdersRemainsInBook)
{
    Order sell = create_order(1, 100, 5, Side::Sell);
    auto trades_sell = engine.submit(&sell);
    EXPECT_EQ(trades_sell.size(), 0u);

    Order buy = create_order(2, 100, 8, Side::Buy);
    auto trades_buy = engine.submit(&buy);
    EXPECT_EQ(trades_buy.size(), 1u);

    // Buy partially filled, should remain in book
    EXPECT_EQ(engine.order_book().size(), 1u);
    auto best_buy = engine.order_book().best_bid_order();
    EXPECT_EQ(best_buy->remaining_quantity, 3u);
}
