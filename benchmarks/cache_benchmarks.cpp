#include "cache/cache.h"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

constexpr std::size_t OPERATIONS = 100000;
constexpr std::size_t KEY_COUNT = 10000;

template <typename Function>
void runBenchmark(
    const std::string& name,
    std::size_t operations,
    Function operation
) {
    const auto start = Clock::now();

    for (std::size_t i = 0; i < operations; ++i) {
        operation(i);
    }

    const auto end = Clock::now();

    const std::chrono::duration<double> elapsed = end - start;

    const double seconds = elapsed.count();
    const double throughput = operations / seconds;

    std::cout << "\n" << name << '\n';
    std::cout << "Operations: " << operations << '\n';
    std::cout << "Time:       " << std::fixed
              << std::setprecision(6) << seconds << " seconds\n";
    std::cout << "Throughput: " << std::fixed
              << std::setprecision(2) << throughput
              << " operations/second\n";
}

int main() {
    std::cout << "====================================\n";
    std::cout << "       CacheForge Benchmark\n";
    std::cout << "====================================\n";

    std::vector<std::string> keys;
    std::vector<std::string> values;

    keys.reserve(KEY_COUNT);
    values.reserve(KEY_COUNT);

    for (std::size_t i = 0; i < KEY_COUNT; ++i) {
        keys.push_back("key-" + std::to_string(i));
        values.push_back("value-" + std::to_string(i));
    }

    Cache writeCache(OPERATIONS);

    runBenchmark("Unique Key Insertions", OPERATIONS,
        [&](std::size_t i) {
            writeCache.set(
                "write-" + std::to_string(i),
                "value"
            );
        }
    );

    Cache readCache(KEY_COUNT);

    for (std::size_t i = 0; i < KEY_COUNT; ++i) {
        readCache.set(keys[i], values[i]);
    }

    std::size_t readChecksum = 0;

    runBenchmark("Repeated Cache Reads", OPERATIONS,
        [&](std::size_t i) {
            const auto result = readCache.get(keys[i % KEY_COUNT]);

            if (result.has_value()) {
                readChecksum += result->size();
            }
        }
    );

    Cache mixedCache(KEY_COUNT);

    for (std::size_t i = 0; i < KEY_COUNT; ++i) {
        mixedCache.set(keys[i], values[i]);
    }

    std::size_t mixedChecksum = 0;

    runBenchmark("Mixed Read/Write Workload", OPERATIONS,
        [&](std::size_t i) {
            const std::size_t index = i % KEY_COUNT;

            if (i % 5 == 0) {
                mixedCache.set(keys[index], "updated-value");
            } else {
                const auto result = mixedCache.get(keys[index]);

                if (result.has_value()) {
                    mixedChecksum += result->size();
                }
            }
        }
    );

    std::cout << "\n====================================\n";
    std::cout << "          Cache Statistics\n";
    std::cout << "====================================\n";

    const CacheStats writeStats = writeCache.getStats();
    const CacheStats readStats = readCache.getStats();
    const CacheStats mixedStats = mixedCache.getStats();

    std::cout << "\nWrite workload:\n";
    std::cout << "Insertions: " << writeStats.insertions << '\n';

    std::cout << "\nRead workload:\n";
    std::cout << "Hits: " << readStats.hits << '\n';
    std::cout << "Misses: " << readStats.misses << '\n';
    std::cout << "Hit rate: " << readStats.hitRate() << "%\n";

    std::cout << "\nMixed workload:\n";
    std::cout << "Hits: " << mixedStats.hits << '\n';
    std::cout << "Insertions: " << mixedStats.insertions << '\n';
    std::cout << "Hit rate: " << mixedStats.hitRate() << "%\n";

    std::cout << "\nChecksums: "
              << readChecksum << ", " << mixedChecksum << '\n';

    return 0;
}
