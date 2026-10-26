#pragma once

#include <cstddef>
#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

struct CacheEntry {
    std::string value;
    std::optional<std::chrono::steady_clock::time_point> expiresAt;
};

class Cache {
public:
    explicit Cache(std::size_t capacity = 1000);

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
    const std::size_t capacity;
};
