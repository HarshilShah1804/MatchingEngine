#pragma once

#include <vector>

#include "matching_engine/book/OrderBook.hpp"
#include "matching_engine/core/Trade.hpp"

namespace matching_engine::engine {

class MatchingEngine final {
public:
    MatchingEngine() = default;

    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;

    MatchingEngine(MatchingEngine&&) = delete;
    MatchingEngine& operator=(MatchingEngine&&) = delete;

    [[nodiscard]]
    std::vector<core::Trade> submit(core::Order* order);

    [[nodiscard]]
    const book::OrderBook& order_book() const noexcept;

private:
    [[nodiscard]]
    bool crosses(const core::Order& incoming,
                 const core::Order& resting) const noexcept;

    book::OrderBook order_book_;
};

} // namespace matching_engine::engine