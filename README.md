# CacheForge

**A thread-safe, in-memory key-value cache engine built with C++17, featuring TTL expiration, LRU eviction, cache statistics, file-based persistence, and an interactive command-line interface.**

CacheForge is a lightweight caching engine designed to explore how in-memory storage systems work internally. It combines hash-map-based storage with an intrusive doubly linked list for efficient Least Recently Used (LRU) tracking.

## Features

- **Fast Key-Value Storage:** Uses `std::unordered_map` for average O(1) lookup, insertion, and deletion.
- **TTL Expiration:** Supports optional time-to-live values for individual entries.
- **Lazy Expiration:** Expired entries are automatically removed when accessed.
- **Explicit Cleanup:** Removes expired entries through `cleanupExpired()`.
- **LRU Eviction:** Automatically evicts the least recently used entry when capacity is reached.
- **Intrusive LRU Tracking:** Uses pointers within cache entries to maintain the doubly linked list.
- **Configurable Capacity:** Specify the maximum cache capacity during construction or CLI launch.
- **Thread Safety:** Protects public cache operations using mutex-based synchronization.
- **Cache Statistics:** Tracks hits, misses, insertions, evictions, and hit rate.
- **File Persistence:** Save cache contents to a file and restore them later.
- **Persistence Validation:** Checks file format, entry count, TTL values, duplicate keys, and malformed input.
- **Interactive CLI:** Provides commands to interact with the cache.
- **Automated Testing:** Includes tests for core operations, TTL, LRU, concurrency, statistics, and persistence.
- **Comparative Benchmarks:** Measures CacheForge against `std::unordered_map` across different workloads.
- **Installable CMake Library:** Supports integration into external C++ projects using `find_package()`.

## Architecture

CacheForge uses two primary data structures:

| Component | Responsibility |
|---|---|
| `std::unordered_map` | Stores keys and cache entries |
| Intrusive doubly linked list | Maintains LRU usage order |

The most recently used entry is maintained at the head of the list, while the least recently used entry is maintained at the tail.

When an entry is accessed or updated, it moves to the head. When the cache reaches its capacity and a new entry is inserted, the tail entry is evicted.

### Cache operation flow

1. A key-value pair is inserted into the hash map.
2. The corresponding cache entry is attached to the front of the LRU list.
3. Successful reads refresh the entry's LRU position.
4. Expired entries are removed when accessed or during explicit cleanup.
5. When capacity is reached, the least recently used entry is evicted.

## Project Structure

```text
CacheForge/
├── cmake/
│   └── CacheForgeConfig.cmake.in
├── examples/
│   └── embedded_cache.cpp
├── include/
│   └── cache/
│       ├── cache.h
│       └── persistence.h
├── src/
│   ├── cache/
│   │   ├── cache.cpp
│   │   └── persistence.cpp
│   └── main.cpp
├── tests/
│   └── cache_tests.cpp
├── benchmarks/
│   └── cache_benchmarks.cpp
├── CMakeLists.txt
├── README.md
└── .gitignore
```

## Getting Started

### Prerequisites

- C++17 compatible compiler
- CMake 3.16 or later
- MSVC, GCC, or Clang
- Git

### Clone the Repository

```bash
git clone https://github.com/MoNiMoNiSh18/CacheForge.git
cd CacheForge
```

### Build with CMake

```bash
cmake -S . -B build-release
cmake --build build-release --config Release
```

### Run the Application

On Windows with the Visual Studio CMake generator:

```cmd
.\build-release\Release\cacheforge.exe
```

The default cache capacity is 1000 entries.

To specify a custom capacity:

```cmd
.\build-release\Release\cacheforge.exe 500
```

This starts CacheForge with a maximum capacity of 500 entries.

The application rejects zero and invalid capacity arguments.

## Interactive CLI

| Command | Description |
|---|---|
| `SET key value` | Insert or update a key-value pair |
| `SET key value ttl` | Insert a key-value pair with TTL in seconds |
| `GET key` | Retrieve a value |
| `DELETE key` | Remove a key |
| `SIZE` | Display the current number of entries |
| `CLEANUP` | Remove expired entries |
| `STATS` | Display cache statistics |
| `SAVE filename` | Save cache contents to a file |
| `LOAD filename` | Restore cache contents from a file |
| `HELP` | Display available commands |
| `EXIT` | Exit the application |

### Example Session

```text
CacheForge> SET username monish
Stored key: username

CacheForge> GET username
Value: monish

CacheForge> SET greeting "hello world"
Stored key: greeting

CacheForge> SET session abc123 60
Stored key: session

CacheForge> SIZE
Current cache size: 3

CacheForge> SAVE cache.dat
Cache saved successfully.

CacheForge> STATS
========== Cache Statistics ==========
Hits:       1
Misses:     0
Insertions: 3
Evictions:  0
Hit rate:   100%
======================================

CacheForge> EXIT
```

Values containing spaces can be entered using double quotes.

A TTL of zero means the entry does not expire automatically.

## LRU Eviction Demonstration

Launch CacheForge with a capacity of three:

```cmd
.\build-release\Release\cacheforge.exe 3
```

Run:

```text
SET A apple
SET B banana
SET C cherry
GET A
SET D dragonfruit
GET B
```

Since accessing A refreshes its LRU position, B becomes the least recently used entry. Inserting D evicts B.

This demonstrates automatic capacity management through LRU eviction.

## File Persistence

CacheForge supports saving and restoring cache contents through `CachePersistence`.

### Save a cache

```text
CacheForge> SET username monish
CacheForge> SAVE cache.dat
```

### Load a cache

```text
CacheForge> LOAD cache.dat
```

### Persistence behaviour

- Uses a versioned file format with the `CACHEFORGE 1` header.
- Saves entries in most-recently-used to least-recently-used order.
- Stores the remaining TTL of entries.
- Remaining TTL is effectively paused while the cache is closed.
- Restores the LRU ordering when loading.
- When loading into a smaller-capacity cache, retains the most recently used entries.
- Resets cache statistics after a successful load.
- Validates file contents before replacing the existing cache state.
- Uses a temporary file and replacement operation during saving to reduce the risk of leaving a partially written destination file.

## Core API

CacheForge can also be integrated into independent C++ applications.

```cpp
#include <cache/cache.h>

#include <chrono>
#include <iostream>

int main() {
    Cache cache(100);

    cache.set("name", "CacheForge");
    cache.set("session", "active", std::chrono::seconds(60));

    auto value = cache.get("name");

    if (value.has_value()) {
        std::cout << *value << '\n';
    }

    cache.remove("name");

    std::cout << "Cache size: " << cache.size() << '\n';

    return 0;
}
```

### Available Operations

| Method | Description |
|---|---|
| `set(key, value, ttl)` | Insert or update an entry |
| `get(key)` | Retrieve an entry as `std::optional<std::string>` |
| `remove(key)` | Remove an entry |
| `size()` | Return the current entry count |
| `cleanupExpired()` | Remove expired entries |
| `getStats()` | Return a snapshot of cache statistics |

## Using CacheForge as an Installed CMake Library

CacheForge can be installed and consumed by another CMake-based C++ project.

### Install CacheForge

From the CacheForge project directory:

```cmd
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
cmake --install build-release --config Release --prefix install
```

### Integrate into another project

In the consumer project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)

project(MyApplication LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CacheForge 1.0 CONFIG REQUIRED)

add_executable(my_app main.cpp)

target_link_libraries(my_app
    PRIVATE CacheForge::cacheforge
)
```

Configure the consumer project with the CacheForge installation prefix:

```cmd
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/path/to/CacheForge/install"
cmake --build build --config Release
```

Then include the public headers:

```cpp
#include <cache/cache.h>
#include <cache/persistence.h>
```

## Cache Statistics

CacheForge maintains the following metrics:

| Metric | Description |
|---|---|
| Hits | Successful cache retrievals |
| Misses | Missing or expired key retrievals |
| Insertions | New entries inserted |
| Evictions | Entries removed due to capacity limits |
| Hit Rate | Percentage of successful retrievals |

Hit rate is calculated as:

```text
Hit Rate = (Hits / (Hits + Misses)) * 100
```

When no retrievals have occurred, the hit rate is zero.

## Testing

The automated test suite covers:

- Basic insertion and retrieval
- Missing keys and removal
- Overwriting existing entries
- TTL expiration
- Explicit expired-entry cleanup
- Capacity validation
- LRU eviction and refresh
- Multiple evictions and statistics
- Concurrent access
- Cache hit-rate calculations
- LRU list removal from different positions
- Persistence save and load
- LRU restoration
- Capacity handling during loading
- Corrupted, truncated, and duplicate-entry files
- Remaining TTL restoration
- Failed-save handling
- Persistence input validation

Run the tests using:

```cmd
cmake --build build-release --config Release
ctest --test-dir build-release -C Release --output-on-failure
```

To execute the test program directly:

```cmd
.\build-release\Release\cache_tests.exe
```

The current development test run reports **32 passed, 0 failed**.

## Benchmarking

CacheForge includes a separate benchmark executable comparing its performance with `std::unordered_map`.

### Benchmark Workloads

1. **Unique Insertions:** Measures insertion throughput using unique keys.
2. **Repeated Reads:** Measures retrieval throughput across an existing key set.
3. **Mixed Workload:** Measures a workload consisting of 80% reads and 20% updates.

### Methodology

- 100,000 operations per workload
- 10,000 keys for read and mixed workloads
- One warm-up run per benchmark
- Five measured repetitions
- Median execution time used for throughput reporting
- String-copy costs included in the standard map read and mixed baselines to better match the cache retrieval interface

Run the benchmark:

```cmd
cmake --build build-release --config Release
.\build-release\Release\cache_benchmarks.exe
```

### Representative Benchmark Results

Release-mode measurements from the development machine:

| Workload | `std::unordered_map` | CacheForge |
|---|---:|---:|
| Unique Insertions | 2,838,393 ops/s | 2,860,797 ops/s |
| Repeated Reads | 22,162,629 ops/s | 12,745,835 ops/s |
| Mixed Workload | 27,982,203 ops/s | 10,366,347 ops/s |

All corresponding benchmark checksums matched.

### Benchmark Environment

- Processor: AMD Ryzen 5 3500U with Radeon Vega Mobile Gfx
- Memory: 8 GB RAM
- Operating System: Windows 11
- Language Standard: C++17
- Build Configuration: Release
- Build System: CMake 4.4.3
- Benchmark Repetitions: 5

**Note:** `std::unordered_map` is a baseline, not a feature-equivalent cache implementation. CacheForge performs additional work for LRU tracking, synchronization, statistics, and TTL management. Results are specific to the development environment and workload and should not be interpreted as a universal performance comparison.

## Technical Concepts Explored

- Hash tables and average O(1) operations
- Doubly linked lists and intrusive data structures
- LRU cache eviction
- TTL and monotonic clocks
- Iterators, pointers, and object lifetime
- `std::optional`
- Mutexes and thread safety
- File persistence and validation
- CMake library packaging and installation
- Automated testing and benchmarking
- Performance measurement and optimization

## Future Improvements

Potential future directions:

- REST API integration
- Cache monitoring interface
- Configurable eviction policies
- Performance and memory profiling
- Additional concurrency and stress testing

## Author

**Monish V.**

- GitHub: [MoNiMoNiSh18](https://github.com/MoNiMoNiSh18)
- Project: [CacheForge](https://github.com/MoNiMoNiSh18/CacheForge)

---

*CacheForge — Exploring the engineering behind efficient in-memory caching systems.*
