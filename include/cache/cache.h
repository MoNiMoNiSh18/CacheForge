#pragma once

#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

struct CacheEntry {
    std::string value;

    std::optional<std::chrono::steady_clock::time_point> expiresAt;

    const std::string* key = nullptr;

    CacheEntry* previous = nullptr;
    CacheEntry* next = nullptr;
};

struct CacheStats {
    std::size_t hits = 0;
    std::size_t misses = 0;
    std::size_t insertions = 0;
    std::size_t evictions = 0;

    double hitRate() const noexcept {
        const std::size_t total = hits + misses;

        if (total == 0) {
            return 0.0;
        }

        return (static_cast<double>(hits) / total) * 100.0;
    }
};

class CachePersistence;

class Cache {
public:
    explicit Cache(std::size_t capacity = 1000);

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

    CacheStats getStats() const;

private:
    std::unordered_map<std::string, CacheEntry> data;
    friend class CachePersistence;
    const std::size_t capacity;

    CacheStats stats;

    CacheEntry* head = nullptr;
    CacheEntry* tail = nullptr;

    mutable std::mutex mutex;

    void attachToFront(CacheEntry* entry) noexcept;
    void detach(CacheEntry* entry) noexcept;
    void moveToFront(CacheEntry* entry) noexcept;
    void evictLeastRecentlyUsed();

    std::size_t cleanupExpiredUnlocked();
};
