#pragma once

namespace matching_engine::core {
    enum class Side {
        Buy,
        Sell
    };

    enum class OrderType {
        Limit,
        Market
    };

    enum class OrderStatus {
        Active,
        PartiallyFilled,
        Filled,
        Cancelled
    };
} // namespace matching_engine