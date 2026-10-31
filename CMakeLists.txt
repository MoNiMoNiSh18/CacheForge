cmake_minimum_required(VERSION 3.16)

project(CacheForge
    VERSION 1.0.0
    DESCRIPTION "A thread-safe in-memory key-value cache engine written in C++17"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Threads REQUIRED)

add_library(cacheforge_lib
    src/cache/cache.cpp
    src/cache/persistence.cpp
)

add_library(CacheForge::cacheforge ALIAS cacheforge_lib)

set_target_properties(cacheforge_lib PROPERTIES
    EXPORT_NAME cacheforge
)

target_compile_features(cacheforge_lib PUBLIC cxx_std_17)

target_include_directories(cacheforge_lib
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(cacheforge_lib
    PUBLIC Threads::Threads
)

add_executable(cacheforge
    src/main.cpp
)

target_link_libraries(cacheforge
    PRIVATE CacheForge::cacheforge
)

enable_testing()

add_executable(cache_tests
    tests/cache_tests.cpp
)

target_link_libraries(cache_tests
    PRIVATE CacheForge::cacheforge
)

add_test(
    NAME CacheForgeTests
    COMMAND cache_tests
)

add_executable(cache_benchmarks
    benchmarks/cache_benchmarks.cpp
)

target_link_libraries(cache_benchmarks
    PRIVATE CacheForge::cacheforge
)

add_executable(cacheforge_example
    examples/embedded_cache.cpp
)

target_link_libraries(cacheforge_example
    PRIVATE CacheForge::cacheforge
)

install(
    TARGETS cacheforge_lib
    EXPORT CacheForgeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    TARGETS cacheforge
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT CacheForgeTargets
    FILE CacheForgeTargets.cmake
    NAMESPACE CacheForge::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CacheForge
)

configure_package_config_file(
    cmake/CacheForgeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CacheForgeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CacheForge
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/CacheForgeConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/CacheForgeConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/CacheForgeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CacheForge
)
