#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>

#include "matching_engine/book/PriceLevel.hpp"
#include "matching_engine/core/Order.hpp"

namespace matching_engine::book {
    class OrderBook final {
    public:
        explicit OrderBook() = default;

        OrderBook(const OrderBook&) = delete;
        OrderBook& operator=(const OrderBook&) = delete;

        OrderBook(OrderBook&&) = delete;
        OrderBook& operator=(OrderBook&&) = delete;

        bool insert(core::Order* order);
        bool cancel(core::OrderID order_id) noexcept;

        [[nodiscard]]
        core::Order* best_bid_order() const noexcept;

        [[nodiscard]]
        core::Order* best_ask_order() const noexcept;

        [[nodiscard]]
        std::optional<core::Price> best_bid_price() const noexcept;

        [[nodiscard]]
        std::optional<core::Price> best_ask_price() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

#ifndef NDEBUG
        void validate() const;
#endif

    private:
        using BidLevels = std::map<core::Price, std::unique_ptr<PriceLevel>, std::greater<core::Price>>;
        using AskLevels = std::map<core::Price, std::unique_ptr<PriceLevel>>;

        struct OrderLocation final {
            core::Side side {};
            core::Price price {};
            PriceLevel* level {nullptr};
            core::Order* order {nullptr};
        };

        [[nodiscard]]
        PriceLevel& level_for(core::Side side, core::Price price);

        [[nodiscard]]
        const PriceLevel* best_level(core::Side side) const noexcept;

        void erase_level_if_empty(core::Side side, core::Price price);

        BidLevels bid_levels_;
        AskLevels ask_levels_;
        std::unordered_map<core::OrderID, OrderLocation> order_index_;
    };
} // namespace matching_engine::book