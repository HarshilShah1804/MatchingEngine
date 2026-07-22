#include "matching_engine/book/OrderBook.hpp"

#include <cassert>

namespace matching_engine::book {

bool OrderBook::insert(core::Order* order)
{
    assert(order != nullptr);
    assert(order->prev == nullptr);
    assert(order->next == nullptr);

    if (order == nullptr || order->type != core::OrderType::Limit)
    {
        return false;
    }

    if (order_index_.find(order->id) != order_index_.end())
    {
        return false;
    }

    PriceLevel& level = level_for(order->side, order->price);
    level.push_back(order);

    order_index_.emplace(order->id, OrderLocation{
        .side = order->side,
        .price = order->price,
        .level = &level,
        .order = order,
    });

#ifndef NDEBUG
    validate();
#endif

    return true;
}

bool OrderBook::cancel(core::OrderID order_id) noexcept
{
    auto location_it = order_index_.find(order_id);
    if (location_it == order_index_.end())
    {
        return false;
    }

    OrderLocation location = location_it->second;
    location.level->erase(location.order);
    order_index_.erase(location_it);
    erase_level_if_empty(location.side, location.price);

#ifndef NDEBUG
    validate();
#endif

    return true;
}

core::Order* OrderBook::best_bid_order() const noexcept
{
    const PriceLevel* level = best_level(core::Side::Buy);
    return level == nullptr ? nullptr : level->front();
}

core::Order* OrderBook::best_ask_order() const noexcept
{
    const PriceLevel* level = best_level(core::Side::Sell);
    return level == nullptr ? nullptr : level->front();
}

std::optional<core::Price> OrderBook::best_bid_price() const noexcept
{
    const auto level_it = bid_levels_.begin();
    if (level_it == bid_levels_.end())
    {
        return std::nullopt;
    }

    return level_it->first;
}

std::optional<core::Price> OrderBook::best_ask_price() const noexcept
{
    const auto level_it = ask_levels_.begin();
    if (level_it == ask_levels_.end())
    {
        return std::nullopt;
    }

    return level_it->first;
}

bool OrderBook::empty() const noexcept
{
    return order_index_.empty();
}

std::size_t OrderBook::size() const noexcept
{
    return order_index_.size();
}

PriceLevel& OrderBook::level_for(core::Side side, core::Price price)
{
    if (side == core::Side::Buy)
    {
        auto [it, inserted] = bid_levels_.try_emplace(price);
        if (inserted || it->second == nullptr)
        {
            it->second = std::make_unique<PriceLevel>();
        }

        return *it->second;
    }

    auto [it, inserted] = ask_levels_.try_emplace(price);
    if (inserted || it->second == nullptr)
    {
        it->second = std::make_unique<PriceLevel>();
    }

    return *it->second;
}

const PriceLevel* OrderBook::best_level(core::Side side) const noexcept
{
    if (side == core::Side::Buy)
    {
        const auto level_it = bid_levels_.begin();
        return level_it == bid_levels_.end() ? nullptr : level_it->second.get();
    }

    const auto level_it = ask_levels_.begin();
    return level_it == ask_levels_.end() ? nullptr : level_it->second.get();
}

void OrderBook::erase_level_if_empty(core::Side side, core::Price price)
{
    if (side == core::Side::Buy)
    {
        auto level_it = bid_levels_.find(price);
        if (level_it != bid_levels_.end() && level_it->second->empty())
        {
            bid_levels_.erase(level_it);
        }

        return;
    }

    auto level_it = ask_levels_.find(price);
    if (level_it != ask_levels_.end() && level_it->second->empty())
    {
        ask_levels_.erase(level_it);
    }
}

#ifndef NDEBUG

void OrderBook::validate() const
{
    std::size_t counted_orders = 0;

    for (const auto& [price, level] : bid_levels_)
    {
        assert(level != nullptr);
        assert(!level->empty());

        for (const core::Order* current = level->front(); current != nullptr; current = current->next)
        {
            assert(current->side == core::Side::Buy);
            assert(current->price == price);
            ++counted_orders;
        }
    }

    for (const auto& [price, level] : ask_levels_)
    {
        assert(level != nullptr);
        assert(!level->empty());

        for (const core::Order* current = level->front(); current != nullptr; current = current->next)
        {
            assert(current->side == core::Side::Sell);
            assert(current->price == price);
            ++counted_orders;
        }
    }

    assert(counted_orders == order_index_.size());
}

#endif

} // namespace matching_engine::book