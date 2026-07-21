#pragma once

#include "matching_engine/core/Types.hpp"
#include "matching_engine/core/Enums.hpp"

namespace matching_engine::core {
    struct Order final {
        OrderID id {};
        SymbolID symbol {};
        Side side {Side::Buy};
        OrderType type {OrderType::Limit};
        Price price {};
        Quantity initial_quantity {};
        Quantity remaining_quantity {};
        TimeStamp timestamp {};

        Order *prev {nullptr};
        Order *next {nullptr};
    };
} // namespace matching_engine