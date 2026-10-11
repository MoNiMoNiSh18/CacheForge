#include "cache/cache.h"

void Cache::set(const std::string& key, const std::string& value) {
    data[key] = value;
}

std::optional<std::string> Cache::get(const std::string& key) const {
    auto it = data.find(key);

    if (it != data.end()) {
        return it->second;
    }

    return std::nullopt;
}

bool Cache::remove(const std::string& key) {
    return data.erase(key) > 0;
}