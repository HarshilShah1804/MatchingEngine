#include "matching_engine/book/PriceLevel.hpp"

#include <cassert>

namespace matching_engine::book
{

void PriceLevel::push_back(core::Order* order) noexcept
{
    assert(order != nullptr);
    assert(order->prev == nullptr);
    assert(order->next == nullptr);

    order->prev = tail_;
    order->next = nullptr;

    if (tail_ != nullptr)
    {
        tail_->next = order;
    }
    else
    {
        head_ = order;
    }

    tail_ = order;
    ++size_;

#ifndef NDEBUG
    validate();
#endif
}

void PriceLevel::erase(core::Order* order) noexcept
{
    assert(order != nullptr);
    assert(!empty());

#ifndef NDEBUG
    bool found = false;
    for (core::Order* current = head_; current != nullptr; current = current->next) {
        if (current == order) {
            found = true;
            break;
        }
    }
    assert(found);
#endif

    if (order->prev != nullptr)
    {
        order->prev->next = order->next;
    }
    else
    {
        head_ = order->next;
    }

    if (order->next != nullptr)
    {
        order->next->prev = order->prev;
    }
    else
    {
        tail_ = order->prev;
    }

    order->prev = nullptr;
    order->next = nullptr;

    --size_;

#ifndef NDEBUG
    validate();
#endif
}

core::Order* PriceLevel::front() const noexcept
{
    return head_;
}

core::Order* PriceLevel::back() const noexcept
{
    return tail_;
}

bool PriceLevel::empty() const noexcept
{
    return size_ == 0;
}

std::size_t PriceLevel::size() const noexcept
{
    return size_;
}

#ifndef NDEBUG

void PriceLevel::validate() const
{
    if (head_ == nullptr)
    {
        assert(tail_ == nullptr);
        assert(size_ == 0);
        return;
    }

    assert(head_->prev == nullptr);
    assert(tail_ != nullptr);
    assert(tail_->next == nullptr);

    std::size_t count = 0;

    const core::Order* current = head_;
    const core::Order* previous = nullptr;

    while (current != nullptr)
    {
        assert(current->prev == previous);

        previous = current;

        ++count;
        assert(count <= size_);

        current = current->next;
    }

    assert(previous == tail_);
    assert(count == size_);
}

#endif

} // namespace matching_engine::book