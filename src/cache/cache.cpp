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

    data.reserve(this->capacity);
}

void Cache::attachToFront(CacheEntry* entry) noexcept {
    entry->previous = nullptr;
    entry->next = head;

    if (head != nullptr) {
        head->previous = entry;
    } else {
        tail = entry;
    }

    head = entry;
}

void Cache::detach(CacheEntry* entry) noexcept {
    if (entry->previous != nullptr) {
        entry->previous->next = entry->next;
    } else {
        head = entry->next;
    }

    if (entry->next != nullptr) {
        entry->next->previous = entry->previous;
    } else {
        tail = entry->previous;
    }

    entry->previous = nullptr;
    entry->next = nullptr;
}

void Cache::moveToFront(CacheEntry* entry) noexcept {
    if (head == entry) {
        return;
    }

    detach(entry);
    attachToFront(entry);
}

void Cache::evictLeastRecentlyUsed() {
    if (tail == nullptr) {
        return;
    }

    CacheEntry* entry = tail;
    const std::string* key = entry->key;

    detach(entry);
    data.erase(*key);

    ++stats.evictions;
}

void Cache::set(
    const std::string& key,
    const std::string& value,
    std::chrono::seconds ttl
) {
    std::lock_guard<std::mutex> lock(mutex);

    std::optional<std::chrono::steady_clock::time_point> expiration;

    if (ttl != std::chrono::seconds::zero()) {
        expiration = std::chrono::steady_clock::now() + ttl;
    }

    auto it = data.find(key);

    if (it != data.end()) {
        it->second.value = value;
        it->second.expiresAt = expiration;

        moveToFront(&it->second);
        return;
    }

    auto [newIt, inserted] = data.emplace(
        key,
        CacheEntry{value, expiration}
    );

    if (!inserted) {
        return;
    }

    CacheEntry* entry = &newIt->second;

    entry->key = &newIt->first;

    attachToFront(entry);

    ++stats.insertions;

    if (data.size() > capacity) {
        evictLeastRecentlyUsed();
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

        detach(&it->second);
        data.erase(it);

        ++stats.misses;
        return std::nullopt;
    }

    moveToFront(&it->second);

    ++stats.hits;

    return it->second.value;
}

bool Cache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = data.find(key);

    if (it == data.end()) {
        return false;
    }

    detach(&it->second);
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

    const auto now = std::chrono::steady_clock::now();

    auto it = data.begin();

    while (it != data.end()) {
        const auto& expiration = it->second.expiresAt;

        if (expiration.has_value() && now >= expiration.value()) {
            detach(&it->second);
            it = data.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }

    return removed;
}

CacheStats Cache::getStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stats;
}
