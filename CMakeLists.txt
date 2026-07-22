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

add_library(matching_engine
    src/book/PriceLevel.cpp
    src/book/OrderBook.cpp
)

target_include_directories(
    matching_engine
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
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

include(GoogleTest)

gtest_discover_tests(PriceLevelTests)
