#include "cache/cache.h"

#include <stdexcept>

Cache::Cache(std::size_t capacity)
    : capacity(capacity)
{
    if (this->capacity == 0) {
        throw std::invalid_argument(
            "Cache capacity must be greater than zero"
        );
    }
}

void Cache::set(
    const std::string& key,
    const std::string& value,
    std::chrono::seconds ttl
) {
    std::lock_guard<std::mutex> lock(mutex);

    cleanupExpiredUnlocked();

    std::optional<std::chrono::steady_clock::time_point> expiration;

    if (ttl != std::chrono::seconds::zero()) {
        expiration = std::chrono::steady_clock::now() + ttl;
    }

    auto it = data.find(key);

    if (it != data.end()) {
        it->second.value = value;
        it->second.expiresAt = expiration;

        usageOrder.splice(
            usageOrder.begin(),
            usageOrder,
            it->second.lruPosition
        );

        return;
    }

    usageOrder.push_front(key);

    try {
        data.emplace(
            key,
            CacheEntry{value, expiration, usageOrder.begin()}
        );
    } catch (...) {
        usageOrder.pop_front();
        throw;
    }
    ++stats.insertions;

   if (data.size() > capacity) {
    const std::string leastUsedKey = usageOrder.back();

    data.erase(leastUsedKey);
    usageOrder.pop_back();

    ++stats.evictions;
}
}

std::optional<std::string> Cache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = data.find(key);

    if (it == data.end()) {
        ++stats.misses;
        return std::nullopt;
    }
    const auto& expiration = it->second.expiresAt;

    if (expiration.has_value() &&
        std::chrono::steady_clock::now() >= expiration.value()) {

        usageOrder.erase(it->second.lruPosition);
        data.erase(it);
        ++stats.misses;

        return std::nullopt;
    }
    usageOrder.splice(
    usageOrder.begin(),
    usageOrder,
    it->second.lruPosition
);

    ++stats.hits;
    return it->second.value;
        
}

CacheStats Cache::getStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stats;
}

bool Cache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = data.find(key);

    if (it == data.end()) {
        return false;
    }

    usageOrder.erase(it->second.lruPosition);
    data.erase(it);

    return true;
}

std::size_t Cache::size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return data.size();
}

std::size_t Cache::cleanupExpired() {
    std::lock_guard<std::mutex> lock(mutex);
    return cleanupExpiredUnlocked();
}

std::size_t Cache::cleanupExpiredUnlocked() {
    std::size_t removed = 0;
    auto it = data.begin();

    while (it != data.end()) {
        const auto& expiration = it->second.expiresAt;

        if (expiration.has_value() &&
            std::chrono::steady_clock::now() >= expiration.value()) {
            usageOrder.erase(it->second.lruPosition);
            it = data.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }

    return removed;
}
