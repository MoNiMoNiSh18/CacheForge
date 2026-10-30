#include "cache/cache.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using Clock = std::chrono::steady_clock;

constexpr std::size_t OPERATIONS = 100000;
constexpr std::size_t KEY_COUNT = 10000;
constexpr int REPETITIONS = 5;

using TimedResult = std::pair<double, std::size_t>;

std::vector<std::string> keys;
std::vector<std::string> values;

void prepareData() {
    keys.reserve(KEY_COUNT);
    values.reserve(KEY_COUNT);

    for (std::size_t i = 0; i < KEY_COUNT; ++i) {
        keys.push_back("key-" + std::to_string(i));
        values.push_back("value-" + std::to_string(i));
    }
}

template <typename Function>
void runBenchmark(
    const std::string& name,
    std::size_t operations,
    Function benchmark
) {
    std::vector<double> times;
    std::size_t checksum = 0;

    benchmark();

    for (int i = 0; i < REPETITIONS; ++i) {
        const TimedResult result = benchmark();

        times.push_back(result.first);
        checksum = result.second;
    }

    std::sort(times.begin(), times.end());

    const double median = times[REPETITIONS / 2];
    const double minimum = times.front();
    const double maximum = times.back();

    const double throughput = operations / median;

    std::cout << "\n" << name << '\n';

    std::cout << "Median time: "
              << std::fixed << std::setprecision(6)
              << median << " seconds\n";

    std::cout << "Min time:    "
              << minimum << " seconds\n";

    std::cout << "Max time:    "
              << maximum << " seconds\n";

    std::cout << "Throughput:  "
              << std::fixed << std::setprecision(2)
              << throughput << " operations/second\n";

    std::cout << "Checksum:    " << checksum << '\n';
}


TimedResult benchmarkMapWrites() {
    std::unordered_map<std::string, std::string> data;

    const auto start = Clock::now();

    for (std::size_t i = 0; i < OPERATIONS; ++i) {
        data.insert_or_assign(
            "write-" + std::to_string(i),
            "value"
        );
    }

    const auto end = Clock::now();

    return {
        std::chrono::duration<double>(end - start).count(),
        data.size()
    };
}

TimedResult benchmarkCacheWrites() {
    Cache cache(OPERATIONS);

    const auto start = Clock::now();

    for (std::size_t i = 0; i < OPERATIONS; ++i) {
        cache.set(
            "write-" + std::to_string(i),
            "value"
        );
    }

    const auto end = Clock::now();

    return {
        std::chrono::duration<double>(end - start).count(),
        cache.size()
    };
}


TimedResult benchmarkMapReads() {
    std::unordered_map<std::string, std::string> data;

    for (std::size_t i = 0; i < KEY_COUNT; ++i) {
        data.emplace(keys[i], values[i]);
    }

    std::size_t checksum = 0;

    const auto start = Clock::now();

    for (std::size_t i = 0; i < OPERATIONS; ++i) {
        const auto it = data.find(keys[i % KEY_COUNT]);

        if (it != data.end()) {
            std::string result = it->second;
            checksum += result.size();
        }    
    }

    const auto end = Clock::now();

    return {
        std::chrono::duration<double>(end - start).count(),
        checksum
    };
}

TimedResult benchmarkCacheReads() {
    Cache cache(KEY_COUNT);

    for (std::size_t i = 0; i < KEY_COUNT; ++i) {
        cache.set(keys[i], values[i]);
    }

    std::size_t checksum = 0;

    const auto start = Clock::now();

    for (std::size_t i = 0; i < OPERATIONS; ++i) {
        const auto result = cache.get(keys[i % KEY_COUNT]);

        if (result.has_value()) {
            checksum += result->size();
        }
    }

    const auto end = Clock::now();

    return {
        std::chrono::duration<double>(end - start).count(),
        checksum
    };
}


TimedResult benchmarkMapMixed() {
    std::unordered_map<std::string, std::string> data;

    for (std::size_t i = 0; i < KEY_COUNT; ++i) {
        data.emplace(keys[i], values[i]);
    }

    std::size_t checksum = 0;

    const auto start = Clock::now();

    for (std::size_t i = 0; i < OPERATIONS; ++i) {
        const std::size_t index = i % KEY_COUNT;

        if (i % 5 == 0) {
            data.insert_or_assign(keys[index], "updated-value");
        } else {
            const auto it = data.find(keys[index]);

            if (it != data.end()) {
                std::string result = it->second;
                checksum += result.size();
            }
        }
    }

    const auto end = Clock::now();

    return {
        std::chrono::duration<double>(end - start).count(),
        checksum
    };
}

TimedResult benchmarkCacheMixed() {
    Cache cache(KEY_COUNT);

    for (std::size_t i = 0; i < KEY_COUNT; ++i) {
        cache.set(keys[i], values[i]);
    }

    std::size_t checksum = 0;

    const auto start = Clock::now();

    for (std::size_t i = 0; i < OPERATIONS; ++i) {
        const std::size_t index = i % KEY_COUNT;

        if (i % 5 == 0) {
            cache.set(keys[index], "updated-value");
        } else {
            const auto result = cache.get(keys[index]);

            if (result.has_value()) {
                checksum += result->size();
            }
        }
    }

    const auto end = Clock::now();

    return {
        std::chrono::duration<double>(end - start).count(),
        checksum
    };
}


int main() {
    prepareData();

    std::cout << "========================================\n";
    std::cout << "       CacheForge V1.0 Benchmark\n";
    std::cout << "========================================\n";

    std::cout << "Operations:  " << OPERATIONS << '\n';
    std::cout << "Repetitions: " << REPETITIONS << '\n';
    std::cout << "Warm-up:     1 run per benchmark\n";

    std::cout << "\n========== UNIQUE INSERTIONS ==========\n";

    runBenchmark(
        "std::unordered_map",
        OPERATIONS,
        benchmarkMapWrites
    );

    runBenchmark(
        "CacheForge",
        OPERATIONS,
        benchmarkCacheWrites
    );

    std::cout << "\n========== REPEATED READS =============\n";

    runBenchmark(
        "std::unordered_map",
        OPERATIONS,
        benchmarkMapReads
    );

    runBenchmark(
        "CacheForge",
        OPERATIONS,
        benchmarkCacheReads
    );

    std::cout << "\n========== MIXED WORKLOAD =============\n";

    runBenchmark(
        "std::unordered_map",
        OPERATIONS,
        benchmarkMapMixed
    );

    runBenchmark(
        "CacheForge",
        OPERATIONS,
        benchmarkCacheMixed
    );

    std::cout << "\n========================================\n";
    std::cout << "       Benchmark completed\n";
    std::cout << "========================================\n";

    return 0;
}
