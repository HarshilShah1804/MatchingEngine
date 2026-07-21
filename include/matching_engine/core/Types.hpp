#pragma once

#include <cstdint>

namespace matching_engine::core {
    using OrderID = std::uint64_t;
    using SymbolID = std::uint32_t;
    using Price = std::int64_t;
    using Quantity = std::uint32_t;
    using TimeStamp = std::uint64_t;
} // namespace matching_engine::core