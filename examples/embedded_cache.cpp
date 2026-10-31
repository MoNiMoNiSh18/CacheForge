#include "cache/cache.h"

#include <chrono>
#include <iostream>

int main() {
    Cache cache(100);

    cache.set("username", "monish");
    cache.set("language", "cpp");

    cache.set(
        "session",
        "active",
        std::chrono::seconds(30)
    );

    auto username = cache.get("username");

    if (username.has_value()) {
        std::cout << "Username: " << *username << '\n';
    }

    CacheStats stats = cache.getStats();

    std::cout << "Cache size: " << cache.size() << '\n';
    std::cout << "Cache hits: " << stats.hits << '\n';
    std::cout << "Cache misses: " << stats.misses << '\n';
    std::cout << "Hit rate: " << stats.hitRate() << "%\n";

    return 0;
}
