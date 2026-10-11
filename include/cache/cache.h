#pragma once

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
    void set(
    const std::string& key,
    const std::string& value,
    std::chrono::seconds ttl = std::chrono::seconds::zero()
);
    std::optional<std::string> get(const std::string& key) const;

    bool remove(const std::string& key);

private:
    std::unordered_map<std::string, CacheEntry> data;
};
