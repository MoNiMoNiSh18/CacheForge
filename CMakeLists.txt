
cmake_minimum_required(VERSION 3.16)

project(CacheForge
    VERSION 0.2.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cacheforge
    src/main.cpp
    src/cache/cache.cpp
)

target_include_directories(cacheforge
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

enable_testing()

add_executable(cache_tests
    tests/cache_tests.cpp
    src/cache/cache.cpp
)

target_include_directories(cache_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

add_test(
    NAME CacheForgeTests
    COMMAND cache_tests
)
