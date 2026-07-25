#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "matching_engine/engine/MatchingEngine.hpp"

namespace {

using matching_engine::core::Order;
using matching_engine::core::OrderID;
using matching_engine::core::OrderType;
using matching_engine::core::Price;
using matching_engine::core::Quantity;
using matching_engine::core::Side;
using matching_engine::core::SymbolID;
using matching_engine::engine::MatchingEngine;

struct BenchmarkResult final {
    std::uint64_t processed_orders {0};
    std::uint64_t trades {0};
    std::uint64_t traded_quantity {0};
    std::uint64_t open_orders {0};
    double seconds {0.0};
};

struct Shard final {
    MatchingEngine engine;
    std::mutex mtx;
};

[[nodiscard]]
std::uint64_t parse_u64(const char* value, const std::uint64_t fallback)
{
    if (value == nullptr)
    {
        return fallback;
    }

    char* end_ptr = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end_ptr, 10);
    if (end_ptr == value)
    {
        return fallback;
    }

    return static_cast<std::uint64_t>(parsed);
}

[[nodiscard]]
Order make_order(const OrderID id,
                 const SymbolID symbol,
                 const Side side,
                 const Price price,
                 const Quantity quantity,
                 const std::uint64_t ts) noexcept
{
    return Order{
        .id = id,
        .symbol = symbol,
        .side = side,
        .type = OrderType::Limit,
        .price = price,
        .initial_quantity = quantity,
        .remaining_quantity = quantity,
        .timestamp = ts,
        .prev = nullptr,
        .next = nullptr,
    };
}

[[nodiscard]]
std::vector<Order> generate_orders(const std::uint64_t total_orders,
                                   const std::uint32_t symbol_count)
{
    std::vector<Order> orders;
    orders.reserve(static_cast<std::size_t>(total_orders));

    std::uint64_t state = 0x9e3779b97f4a7c15ULL;

    for (std::uint64_t i = 0; i < total_orders; ++i)
    {
        state = state * 6364136223846793005ULL + 1ULL;
        const Side side = (state & 1ULL) == 0ULL ? Side::Buy : Side::Sell;

        const SymbolID symbol = static_cast<SymbolID>((i % symbol_count) + 1U);

        // Keep prices in a narrow band to force frequent crossing and trades.
        state = state * 6364136223846793005ULL + 1ULL;
        const Price price_offset = static_cast<Price>(state % 20ULL);
        const Price base_price = 10'000;
        const Price price = side == Side::Buy
            ? (base_price - price_offset)
            : (base_price - 10 + price_offset);

        state = state * 6364136223846793005ULL + 1ULL;
        const Quantity qty = static_cast<Quantity>((state % 10ULL) + 1ULL);

        orders.push_back(make_order(i + 1ULL, symbol, side, price, qty, i));
    }

    return orders;
}

[[nodiscard]]
BenchmarkResult run_single_thread(std::vector<Order>& orders)
{
    MatchingEngine engine;
    BenchmarkResult result;
    result.processed_orders = static_cast<std::uint64_t>(orders.size());

    const auto start = std::chrono::steady_clock::now();

    for (Order& order : orders)
    {
        auto trades = engine.submit(&order);
        result.trades += static_cast<std::uint64_t>(trades.size());
        for (const auto& trade : trades)
        {
            result.traded_quantity += static_cast<std::uint64_t>(trade.quantity);
        }
    }

    const auto end = std::chrono::steady_clock::now();

    result.seconds = std::chrono::duration<double>(end - start).count();
    result.open_orders = static_cast<std::uint64_t>(engine.order_book().size());
    return result;
}

[[nodiscard]]
BenchmarkResult run_sharded_concurrent(std::vector<Order>& orders,
                                       const std::size_t threads,
                                       const std::size_t shards_count)
{
    std::vector<Shard> shards(shards_count);

    struct WorkerStats final {
        std::uint64_t trades {0};
        std::uint64_t traded_quantity {0};
    };

    std::vector<WorkerStats> worker_stats(threads);
    std::vector<std::thread> workers;
    workers.reserve(threads);

    const std::size_t total = orders.size();
    const std::size_t chunk = (total + threads - 1U) / threads;

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t t = 0; t < threads; ++t)
    {
        const std::size_t begin = t * chunk;
        const std::size_t end = std::min(begin + chunk, total);

        workers.emplace_back([&, t, begin, end]() {
            for (std::size_t i = begin; i < end; ++i)
            {
                Order& order = orders[i];

                // Shard by symbol so each shard can progress in parallel.
                const std::size_t shard_idx = static_cast<std::size_t>(order.symbol) % shards_count;
                Shard& shard = shards[shard_idx];

                std::lock_guard<std::mutex> lock(shard.mtx);
                auto trades = shard.engine.submit(&order);
                worker_stats[t].trades += static_cast<std::uint64_t>(trades.size());
                for (const auto& trade : trades)
                {
                    worker_stats[t].traded_quantity += static_cast<std::uint64_t>(trade.quantity);
                }
            }
        });
    }

    for (auto& worker : workers)
    {
        worker.join();
    }

    const auto end = std::chrono::steady_clock::now();

    BenchmarkResult result;
    result.processed_orders = static_cast<std::uint64_t>(orders.size());
    result.seconds = std::chrono::duration<double>(end - start).count();

    for (const WorkerStats& stats : worker_stats)
    {
        result.trades += stats.trades;
        result.traded_quantity += stats.traded_quantity;
    }

    for (const Shard& shard : shards)
    {
        result.open_orders += static_cast<std::uint64_t>(shard.engine.order_book().size());
    }

    return result;
}

void print_result(const std::string& name, const BenchmarkResult& result)
{
    const double ops_per_sec =
        result.seconds > 0.0
            ? static_cast<double>(result.processed_orders) / result.seconds
            : 0.0;

    std::cout << "=== " << name << " ===\n";
    std::cout << "Processed orders: " << result.processed_orders << '\n';
    std::cout << "Trades:           " << result.trades << '\n';
    std::cout << "Traded quantity:  " << result.traded_quantity << '\n';
    std::cout << "Open orders:      " << result.open_orders << '\n';
    std::cout << "Elapsed seconds:  " << result.seconds << '\n';
    std::cout << "Throughput:       " << static_cast<std::uint64_t>(ops_per_sec)
              << " orders/sec\n\n";
}

} // namespace

int main(int argc, char** argv)
{
    const std::uint64_t total_orders = argc > 1 ? parse_u64(argv[1], 5'000'000ULL) : 5'000'000ULL;
    const std::uint64_t symbols_u64 = argc > 2 ? parse_u64(argv[2], 64ULL) : 64ULL;
    const std::uint64_t threads_u64 = argc > 3
        ? parse_u64(argv[3], static_cast<std::uint64_t>(std::max(1U, std::thread::hardware_concurrency())))
        : static_cast<std::uint64_t>(std::max(1U, std::thread::hardware_concurrency()));

    const std::uint32_t symbol_count = static_cast<std::uint32_t>(std::max<std::uint64_t>(1ULL, symbols_u64));
    const std::size_t threads = static_cast<std::size_t>(std::max<std::uint64_t>(1ULL, threads_u64));
    const std::size_t shards = threads;

    std::cout << "Generating " << total_orders << " orders across " << symbol_count
              << " symbols...\n";

    auto single_orders = generate_orders(total_orders, symbol_count);
    auto sharded_orders = single_orders;

    const BenchmarkResult single = run_single_thread(single_orders);
    print_result("Single-thread", single);

    const BenchmarkResult concurrent = run_sharded_concurrent(sharded_orders, threads, shards);
    print_result("Concurrent sharded", concurrent);

    if (single.seconds > 0.0 && concurrent.seconds > 0.0)
    {
        const double speedup = single.seconds / concurrent.seconds;
        std::cout << "Speedup (concurrent vs single): " << speedup << "x\n";
    }

    return 0;
}
