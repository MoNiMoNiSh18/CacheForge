#include <iostream>
#include <string>
#include "cache/cache.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <thread>
#include <vector>

bool testOverwriteRefreshesLRU() {
    Cache cache(2);

    cache.set("A", "1");
    cache.set("B", "2");

    cache.set("A", "updated");
    cache.set("C", "3");

    return cache.get("A") == std::optional<std::string>("updated") &&
           cache.get("B") == std::nullopt &&
           cache.get("C") == std::optional<std::string>("3");
}

bool testReadRefreshesLRU() {
    Cache cache(2);

    cache.set("A", "1");
    cache.set("B", "2");

    cache.get("A");
    cache.set("C", "3");

    return cache.get("A") == std::optional<std::string>("1") &&
           cache.get("B") == std::nullopt &&
           cache.get("C") == std::optional<std::string>("3");
}

bool testMultipleEvictionsUpdateStatistics() {
    Cache cache(2);

    cache.set("A", "1");
    cache.set("B", "2");
    cache.set("C", "3");
    cache.set("D", "4");

    const CacheStats stats = cache.getStats();

    return cache.size() == 2 &&
           stats.insertions == 4 &&
           stats.evictions == 2;
}

bool testSetAndGet() {
    Cache cache;
    cache.set("user:101", "Monish");

    auto value = cache.get("user:101");

    return value.has_value() && value.value() == "Monish";
}

bool testMissingKey() {
    Cache cache;

    auto value = cache.get("user:999");

    return !value.has_value();
}

bool testOverwrite() {
    Cache cache;

    cache.set("user:101", "Monish");
    cache.set("user:101", "Monish V");

    auto value = cache.get("user:101");

    return value.has_value() && value.value() == "Monish V";
}

bool testRemoveExistingKey() {
    Cache cache;
    cache.set("user:101", "Monish");

    return cache.remove("user:101");
}

bool testRemovedKeyCannotBeRetrieved() {
    Cache cache;
    cache.set("user:101", "Monish");
    cache.remove("user:101");

    return !cache.get("user:101").has_value();
}

bool testRemoveMissingKey() {
    Cache cache;

    return !cache.remove("user:999");
}


bool testEntryExpires() {
    Cache cache;

    cache.set("session:101", "active", std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    return !cache.get("session:101").has_value();
}

bool testEntryWithoutTTLDoesNotExpire() {
    Cache cache;

    cache.set("user:101", "Monish");

    return cache.get("user:101").value_or("") == "Monish";
}

bool testExpiredEntryIsRemovedFromCache() {
    Cache cache;

    cache.set("session:101", "active", std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    if (cache.size() != 1) {
        return false;
    }

    auto value = cache.get("session:101");

    return !value.has_value() && cache.size() == 0;
}


bool testCleanupRemovesMultipleExpiredEntries() {
    Cache cache;

    cache.set("session:1", "A", std::chrono::seconds(1));
    cache.set("session:2", "B", std::chrono::seconds(1));
    cache.set("session:3", "C", std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    const auto removed = cache.cleanupExpired();

    return removed == 3 && cache.size() == 0;
}

bool testCleanupPreservesNonExpiredEntries() {
    Cache cache;

    cache.set("session:expired", "A", std::chrono::seconds(1));
    cache.set("session:active", "B", std::chrono::seconds(30));

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    const auto removed = cache.cleanupExpired();

    auto activeValue = cache.get("session:active");

    return removed == 1 &&
           cache.size() == 1 &&
           activeValue.has_value() &&
           activeValue.value() == "B";
}


bool testCapacityLimit() {
    Cache cache(2);

    cache.set("A", "1");
    cache.set("B", "2");
    cache.set("C", "3");

    return cache.size() == 2 &&
           cache.get("A") == std::nullopt &&
           cache.get("B") == std::optional<std::string>("2") &&
           cache.get("C") == std::optional<std::string>("3");
}


bool testZeroCapacityRejected() {
    try {
        Cache cache(0);
    } catch (const std::invalid_argument&) {
        return true;
    }

    return false;
}

bool testDefaultCapacity() {
    Cache cache;

    cache.set("A", "1");

    return cache.get("A") == std::optional<std::string>("1");
}


bool testLRUEviction() {
    Cache cache(3);

    cache.set("A", "1");
    cache.set("B", "2");
    cache.set("C", "3");

    if (cache.get("A") != std::optional<std::string>("1")) {
        return false;
    }

    cache.set("D", "4");

    return cache.size() == 3 &&
           cache.get("B") == std::nullopt &&
           cache.get("A") == std::optional<std::string>("1") &&
           cache.get("C") == std::optional<std::string>("3") &&
           cache.get("D") == std::optional<std::string>("4");
}

bool testOverwriteWhenFull() {
    Cache cache(2);

    cache.set("A", "1");
    cache.set("B", "2");
    cache.set("A", "updated");

    return cache.size() == 2 &&
           cache.get("A") == std::optional<std::string>("updated") &&
           cache.get("B") == std::optional<std::string>("2");
}

bool testLRUWithTTLAndCleanup() {
    Cache cache(3);

    cache.set("A", "1", std::chrono::seconds(1));
    cache.set("B", "2");
    cache.set("C", "3");

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    if (cache.cleanupExpired() != 1) {
        return false;
    }

    cache.set("D", "4");
    cache.set("E", "5");

    return cache.size() == 3 &&
           cache.get("A") == std::nullopt &&
           cache.get("B") == std::nullopt &&
           cache.get("C") == std::optional<std::string>("3") &&
           cache.get("D") == std::optional<std::string>("4") &&
           cache.get("E") == std::optional<std::string>("5");
}


bool testConcurrentAccess() {
    constexpr int threadCount = 8;
    constexpr int entriesPerThread = 250;

    Cache cache(threadCount * entriesPerThread);
    std::atomic<bool> success{true};
    std::vector<std::thread> workers;

    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < entriesPerThread; ++i) {
                const std::string key =
                    "thread-" + std::to_string(t) +
                    "-key-" + std::to_string(i);

                const std::string expected = std::to_string(i);

                cache.set(key, expected);

                const auto value = cache.get(key);

                if (!value || *value != expected) {
                    success.store(false);
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    if (!success.load() ||
        cache.size() != threadCount * entriesPerThread) {
        return false;
    }

    workers.clear();

    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < entriesPerThread; ++i) {
                const std::string key =
                    "thread-" + std::to_string(t) +
                    "-key-" + std::to_string(i);

                if (!cache.remove(key)) {
                    success.store(false);
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    return success.load() && cache.size() == 0;
}

bool testCacheStatistics() {
    Cache cache(2);

    cache.set("a", "1");
    cache.set("b", "2");

    cache.set("a", "updated");

    cache.get("a");
    cache.get("missing");

    cache.set("c", "3");

    const CacheStats stats = cache.getStats();

    return stats.hits == 1 &&
           stats.misses == 1 &&
           stats.insertions == 3 &&
           stats.evictions == 1;
}

bool testCacheHitRate() {
    Cache cache;

    if (cache.getStats().hitRate() != 0.0) {
        return false;
    }

    cache.set("a", "value");
    cache.get("a");       
    cache.get("missing"); 

    return cache.getStats().hitRate() == 50.0;
}

bool testLRURemovalPositions() {
    Cache cache(4);

    cache.set("A", "1");
    cache.set("B", "2");
    cache.set("C", "3");
    cache.set("D", "4");

    cache.get("B");

    if (!cache.remove("B")) return false; // Head
    if (!cache.remove("C")) return false; // Middle
    if (!cache.remove("A")) return false; // Tail

    if (cache.size() != 1 ||
        cache.get("D") != std::optional<std::string>("4")) {
        return false;
    }

    cache.set("E", "5");
    cache.set("F", "6");
    cache.set("G", "7");
    cache.set("H", "8");

    return cache.size() == 4 &&
           cache.get("D") == std::nullopt &&
           cache.get("E") == std::optional<std::string>("5") &&
           cache.get("F") == std::optional<std::string>("6") &&
           cache.get("G") == std::optional<std::string>("7") &&
           cache.get("H") == std::optional<std::string>("8");
}

int main() {
    int passed = 0;
    int failed = 0;

    auto runTest = [&](const std::string& name, bool (*test)()) {
        if (test()) {
            std::cout << "[PASS] " << name << '\n';
            ++passed;
        } else {
            std::cout << "[FAIL] " << name << '\n';
            ++failed;
        }
    };

    runTest("SET and GET", testSetAndGet);
    runTest("Missing key", testMissingKey);
    runTest("Overwrite existing key", testOverwrite);
    runTest("Remove existing key", testRemoveExistingKey);
    runTest("Retrieve removed key", testRemovedKeyCannotBeRetrieved);
    runTest("Remove missing key", testRemoveMissingKey);
    runTest("Entry expires after TTL", testEntryExpires);
    runTest("Entry without TTL does not expire", testEntryWithoutTTLDoesNotExpire);
    runTest("Expired entry is removed from cache",testExpiredEntryIsRemovedFromCache);
    runTest("Cleanup removes multiple expired entries",testCleanupRemovesMultipleExpiredEntries);
    runTest("Cleanup preserves non-expired entries",testCleanupPreservesNonExpiredEntries);
    runTest("Capacity limit", testCapacityLimit);
    runTest("Overwrite refreshes LRU", testOverwriteRefreshesLRU);
    runTest("Read refreshes LRU", testReadRefreshesLRU);
    runTest("Multiple evictions update statistics",testMultipleEvictionsUpdateStatistics);    
    runTest("Reject zero capacity", testZeroCapacityRejected);
    runTest("Default capacity", testDefaultCapacity);
    runTest("LRU eviction", testLRUEviction);
    runTest("Overwrite when full", testOverwriteWhenFull);
    runTest("LRU with TTL and cleanup", testLRUWithTTLAndCleanup);
    runTest("Concurrent access", testConcurrentAccess);
    runTest("Cache statistics", testCacheStatistics);
    runTest("Cache hit rate", testCacheHitRate);
    runTest("LRU removal positions", testLRURemovalPositions);
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
