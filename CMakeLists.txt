cmake_minimum_required(VERSION 3.16)

project(CacheForge
    VERSION 0.2.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(cacheforge
    src/main.cpp
    src/cache/cache.cpp
)

target_include_directories(cacheforge
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(cache_tests
    tests/cache_tests.cpp
    src/cache/cache.cpp
)

target_include_directories(cache_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(cache_tests PRIVATE Threads::Threads)

enable_testing()

add_test(
    NAME CacheForgeTests
    COMMAND cache_tests
)

add_executable(cache_benchmarks
    benchmarks/cache_benchmarks.cpp
    src/cache/cache.cpp
)

target_include_directories(cache_benchmarks
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(cache_benchmarks
    PRIVATE
        Threads::Threads
)