cmake_minimum_required(VERSION 3.16)
project(HFT_OrderBook CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Compiler flags for maximum performance ────────────────────────────────────
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(include)

# ── Main binary ───────────────────────────────────────────────────────────────
add_executable(orderbook src/main.cpp)
target_compile_options(orderbook PRIVATE -Wall -Wextra -O3)
target_link_libraries(orderbook pthread)

# ── Tests ─────────────────────────────────────────────────────────────────────
add_executable(tests tests/test_orderbook.cpp)
target_compile_options(tests PRIVATE -Wall -Wextra -O2)
target_link_libraries(tests pthread)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
