#include <iostream>
#include <string>
#include "cache/cache.h"
#include <chrono>
#include <thread>

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

    if (cache.size() != 2) {
        return false;
    }

    try {
        cache.set("C", "3");
    } catch (const std::overflow_error&) {
        return cache.size() == 2 &&
               !cache.get("C").has_value();
    }

    return false;
}

bool testOverwriteWhenFull() {
    Cache cache(2);

    cache.set("A", "1");
    cache.set("B", "2");
    cache.set("A", "updated");

    return cache.size() == 2 &&
           cache.get("A") == std::optional<std::string>("updated");
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
    runTest("Overwrite when full", testOverwriteWhenFull);
    runTest("Reject zero capacity", testZeroCapacityRejected);
    runTest("Default capacity", testDefaultCapacity);
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
