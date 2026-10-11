
#include <cassert>
#include <iostream>
#include "cache/cache.h"

int main() {
    Cache cache;

    cache.set("user:101", "Monish");

    auto value = cache.get("user:101");
    assert(value.has_value());
    assert(value.value() == "Monish");

    auto missing = cache.get("user:999");
    assert(!missing.has_value());

    cache.set("user:101", "Monish V");

    auto updated = cache.get("user:101");
    assert(updated.has_value());
    assert(updated.value() == "Monish V");

    assert(cache.remove("user:101"));

    assert(!cache.get("user:101").has_value());

    assert(!cache.remove("user:999"));

    std::cout << "All CacheForge tests passed!\n";

    return 0;
}
