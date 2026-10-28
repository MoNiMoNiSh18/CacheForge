
#pragma once

#include <chrono>
#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

struct CacheEntry {
    std::string value;
    std::optional<std::chrono::steady_clock::time_point> expiresAt;
    std::list<std::string>::iterator lruPosition;
};

class Cache {
public:
    explicit Cache(std::size_t capacity = 1000);

    // A cache contains internal list iterators, so copying is disabled.
    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    void set(
        const std::string& key,
        const std::string& value,
        std::chrono::seconds ttl = std::chrono::seconds::zero()
    );

    std::optional<std::string> get(const std::string& key);

    bool remove(const std::string& key);

    std::size_t size() const;

    std::size_t cleanupExpired();

private:
    std::unordered_map<std::string, CacheEntry> data;
    std::list<std::string> usageOrder;
    const std::size_t capacity;
    std::size_t cleanupExpiredUnlocked();
    mutable std::mutex mutex;
};
