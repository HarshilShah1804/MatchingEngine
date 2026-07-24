#include "matching_engine/engine/MatchingEngine.hpp"

#include <algorithm>
#include <cassert>

namespace matching_engine::engine {

std::vector<core::Trade> MatchingEngine::submit(core::Order* order)
{
    assert(order != nullptr);
    assert(order->prev == nullptr);
    assert(order->next == nullptr);
    assert(order->remaining_quantity > 0);

    std::vector<core::Trade> trades;

    // Only support Limit orders for now
    if (order->type != core::OrderType::Limit)
    {
        return trades;
    }

    // Match incoming order against order book
    if (order->side == core::Side::Buy)
    {
        // For buy orders, match against ask levels (sell orders)
        // Ask levels are sorted in ascending order (best ask first)
        while (order->remaining_quantity > 0)
        {
            core::Order* best_ask = order_book_.best_ask_order();
            if (best_ask == nullptr || !crosses(*order, *best_ask))
            {
                break;
            }

            // Execute trade
            core::Quantity matched_quantity =
                std::min(order->remaining_quantity, best_ask->remaining_quantity);

            trades.push_back(core::Trade{
                .buy_order = order->id,
                .sell_order = best_ask->id,
                .execution_price = best_ask->price,
                .quantity = matched_quantity,
            });

            // Update quantities
            order->remaining_quantity -= matched_quantity;
            best_ask->remaining_quantity -= matched_quantity;

            // Remove fully filled ask order from book
            if (best_ask->remaining_quantity == 0)
            {
                order_book_.cancel(best_ask->id);
            }
        }
    }
    else  // Sell
    {
        // For sell orders, match against bid levels (buy orders)
        // Bid levels are sorted in descending order (best bid first)
        while (order->remaining_quantity > 0)
        {
            core::Order* best_bid = order_book_.best_bid_order();
            if (best_bid == nullptr || !crosses(*order, *best_bid))
            {
                break;
            }

            // Execute trade
            core::Quantity matched_quantity =
                std::min(order->remaining_quantity, best_bid->remaining_quantity);

            trades.push_back(core::Trade{
                .buy_order = best_bid->id,
                .sell_order = order->id,
                .execution_price = best_bid->price,
                .quantity = matched_quantity,
            });

            // Update quantities
            order->remaining_quantity -= matched_quantity;
            best_bid->remaining_quantity -= matched_quantity;

            // Remove fully filled bid order from book
            if (best_bid->remaining_quantity == 0)
            {
                order_book_.cancel(best_bid->id);
            }
        }
    }

    // Add remaining order to book if not fully filled
    if (order->remaining_quantity > 0)
    {
        order_book_.insert(order);
    }

    return trades;
}

const book::OrderBook& MatchingEngine::order_book() const noexcept
{
    return order_book_;
}

bool MatchingEngine::crosses(const core::Order& incoming,
                              const core::Order& resting) const noexcept
{
    // Orders must be on opposite sides
    if (incoming.side == resting.side)
    {
        return false;
    }

    // For a buy order to cross with a sell order: buy_price >= sell_price
    // For a sell order to cross with a buy order: sell_price <= buy_price
    if (incoming.side == core::Side::Buy)
    {
        return incoming.price >= resting.price;
    }

    return incoming.price <= resting.price;
}

}  // namespace matching_engine::engine
