#include <iostream>
#include <string>
#include "cache/cache.h"

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

    std::cout << "\nResults: " << passed
              << " passed, " << failed << " failed\n";

    return failed == 0 ? 0 : 1;
}
