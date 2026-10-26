#include "cache/cache.h"

void Cache::set(
    const std::string& key,
    const std::string& value,
    std::chrono::seconds ttl
) {
    if (ttl == std::chrono::seconds::zero()) {
        data[key] = CacheEntry{value, std::nullopt};
    } else {
        auto expirationTime = std::chrono::steady_clock::now() + ttl;

        data[key] = CacheEntry{value, expirationTime};
    }
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
