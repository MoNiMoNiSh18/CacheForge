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
    std::size_t size() const;
    void set(
    const std::string& key,
    const std::string& value,
    std::chrono::seconds ttl = std::chrono::seconds::zero()
);
    std::optional<std::string> get(const std::string& key);
    bool remove(const std::string& key);

private:
    std::unordered_map<std::string, CacheEntry> data;
};
