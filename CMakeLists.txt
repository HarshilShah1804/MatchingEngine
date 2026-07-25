cmake_minimum_required(VERSION 3.22)

project(
    MatchingEngine
    VERSION 0.1.0
    DESCRIPTION "A High Performance C++ Matching Engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(matching_engine
    src/book/PriceLevel.cpp
    src/book/OrderBook.cpp
    src/engine/MatchingEngine.cpp
)

target_include_directories(
    matching_engine
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(
    MatchingEngineBenchmark
    benchmarks/BenchmarkMatchingEngine.cpp
)

target_link_libraries(
    MatchingEngineBenchmark
    PRIVATE
        matching_engine
        Threads::Threads
)

if(MSVC)
    target_compile_options(matching_engine PRIVATE /W4)
else()
    target_compile_options(
        matching_engine
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
    )
endif()

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    PriceLevelTests
    tests/book/TestPriceLevel.cpp
)

target_link_libraries(
    PriceLevelTests
    PRIVATE
        matching_engine
        GTest::gtest_main
)

add_executable(
    OrderBookTests
    tests/book/TestOrderBook.cpp
)

target_link_libraries(
    OrderBookTests
    PRIVATE
        matching_engine
        GTest::gtest_main
)

add_executable(
    MatchingEngineTests
    tests/engine/TestMatchingEngine.cpp
)

target_link_libraries(
    MatchingEngineTests
    PRIVATE
        matching_engine
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(PriceLevelTests)
gtest_discover_tests(OrderBookTests)
gtest_discover_tests(MatchingEngineTests)
