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
    cleanupExpired();

    CacheEntry entry;

    entry.value = value;

    if (ttl != std::chrono::seconds::zero()) {
        entry.expiresAt =
            std::chrono::steady_clock::now() + ttl;
    } else {
        entry.expiresAt = std::nullopt;
    }

    auto it = data.find(key);

    if (it != data.end()) {
        it->second = entry;
        return;
    }

    if (data.size() >= capacity) {
        throw std::overflow_error("Cache capacity reached");
    }

    data[key] = entry;
}


std::optional<std::string> Cache::get(const std::string& key) {
    auto it = data.find(key);

    if (it == data.end()) {
        return std::nullopt;
    }

    const auto& expiration = it->second.expiresAt;

    if (expiration.has_value() &&
        std::chrono::steady_clock::now() >= expiration.value()) {
        data.erase(it);
        return std::nullopt;
    }

    return it->second.value;
}


bool Cache::remove(const std::string& key) {
    return data.erase(key) > 0;
}

std::size_t Cache::size() const {
    return data.size();
}


std::size_t Cache::cleanupExpired() {
    std::size_t removed = 0;

    auto it = data.begin();

    while (it != data.end()) {
        const auto& expiration = it->second.expiresAt;

        if (expiration.has_value() &&
            std::chrono::steady_clock::now() >= expiration.value()) {
            it = data.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }

    return removed;
}
