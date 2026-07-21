#pragma once

#include "matching_engine/core/Types.hpp"

namespace matching_engine::core {
    struct Trade final {
        OrderID buy_order {};
        OrderID sell_order {};
        Price execution_price {};
        Quantity quantity {};
    };
} // namespace matching_engine::core