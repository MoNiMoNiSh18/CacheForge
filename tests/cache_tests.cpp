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
    std::cout << "\nResults: " << passed
              << " passed, " << failed << " failed\n";

    return failed == 0 ? 0 : 1;
}
