#pragma once

#include <cstddef>

#include "matching_engine/core/Order.hpp"

namespace matching_engine::book {
    class PriceLevel {
    public:
        PriceLevel() = default;
        ~PriceLevel() = default;

        PriceLevel(const PriceLevel&) = delete;
        PriceLevel& operator=(const PriceLevel&) = delete;

        PriceLevel(PriceLevel&&) = delete;
        PriceLevel& operator=(PriceLevel&&) = delete;

        void push_back(core::Order* order) noexcept;
        void erase(core::Order* order) noexcept;

        [[nodiscard]]
        core::Order* front() const noexcept;

        [[nodiscard]]
        core::Order* back() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;
        
#ifndef NDEBUG
        void validate() const;
#endif

    private:
        core::Order* head_ = nullptr;
        core::Order* tail_ = nullptr;
        std::size_t size_ = 0;
    };
} // namespace matching_engine::book