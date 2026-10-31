#include "cache/persistence.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
    constexpr const char* FILE_MAGIC = "CACHEFORGE";
    constexpr int FILE_VERSION = 1;
    constexpr std::size_t MAX_SAVED_ENTRIES = 1000000;

    struct SavedEntry {
        long long remainingTTL;
        std::string key;
        std::string value;
    };

    bool replaceFile(
        const std::filesystem::path& temporary,
        const std::filesystem::path& destination,
        std::string& error
    ) {
#ifdef _WIN32
        if (!MoveFileExW(
                temporary.c_str(),
                destination.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
            )) {
            error = "Unable to replace save file. Windows error: "
                  + std::to_string(GetLastError());
            return false;
        }
#else
        std::error_code ec;
        std::filesystem::rename(temporary, destination, ec);

        if (ec) {
            error = "Unable to replace save file: " + ec.message();
            return false;
        }
#endif

        return true;
    }
}

bool CachePersistence::save(
    const Cache& cache,
    const std::string& filename,
    std::string& error
) {
    struct SnapshotEntry {
        long long remainingTTL;
        std::string key;
        std::string value;
    };

    std::vector<SnapshotEntry> snapshot;

    {
        std::lock_guard<std::mutex> lock(cache.mutex);

        const auto now = std::chrono::steady_clock::now();

        // Preserve MRU-to-LRU ordering.
        for (CacheEntry* entry = cache.head;
             entry != nullptr;
             entry = entry->next) {

            long long remainingTTL = -1;

            if (entry->expiresAt.has_value()) {
                if (*entry->expiresAt <= now) {
                    continue;
                }

                auto remaining =
                    std::chrono::ceil<std::chrono::milliseconds>(
                        *entry->expiresAt - now
                    );

                remainingTTL = remaining.count();
            }

            snapshot.push_back({
                remainingTTL,
                *entry->key,
                entry->value
            });
        }
    }

    const std::filesystem::path destination(filename);
    std::filesystem::path temporary = destination;
    temporary += ".tmp";

    std::ofstream file(
        temporary,
        std::ios::out | std::ios::trunc
    );

    if (!file.is_open()) {
        error = "Unable to open temporary save file: "
              + temporary.string();
        return false;
    }

    auto cleanupTemporary = [&temporary]() {
        std::error_code ec;
        std::filesystem::remove(temporary, ec);
    };

    file << FILE_MAGIC << ' ' << FILE_VERSION << '\n';
    file << snapshot.size() << '\n';

    for (const auto& entry : snapshot) {
        file << entry.remainingTTL << ' '
             << std::quoted(entry.key) << ' '
             << std::quoted(entry.value) << '\n';
    }

    file.flush();

    if (!file.good()) {
        file.close();
        cleanupTemporary();
        error = "Failed while writing temporary cache data.";
        return false;
    }

    file.close();

    if (file.fail()) {
        cleanupTemporary();
        error = "Failed while closing temporary cache file.";
        return false;
    }

    if (!replaceFile(temporary, destination, error)) {
        cleanupTemporary();
        return false;
    }

    error.clear();
    return true;
}

bool CachePersistence::load(
    Cache& cache,
    const std::string& filename,
    std::string& error
) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        error = "Unable to open file: " + filename;
        return false;
    }

    std::string magic;
    int version = 0;
    std::size_t entryCount = 0;

    if (!(file >> magic >> version) ||
        magic != FILE_MAGIC ||
        version != FILE_VERSION) {

        error = "Invalid or unsupported CacheForge file format.";
        return false;
    }

    if (!(file >> entryCount) ||
        entryCount > MAX_SAVED_ENTRIES) {

        error = "Invalid or excessive entry count in save file.";
        return false;
    }

    std::vector<SavedEntry> entries;
    entries.reserve(entryCount);

    std::unordered_set<std::string> seenKeys;
    seenKeys.reserve(entryCount);

    for (std::size_t i = 0; i < entryCount; ++i) {
        SavedEntry entry{};

        if (!(file >> entry.remainingTTL
                   >> std::quoted(entry.key)
                   >> std::quoted(entry.value))) {

            error = "Save file is incomplete or corrupted.";
            return false;
        }

        if (entry.remainingTTL < -1 || entry.remainingTTL == 0) {
            error = "Invalid TTL value in save file.";
            return false;
        }

        if (!seenKeys.insert(entry.key).second) {
            error = "Duplicate key found in save file.";
            return false;
        }

        entries.push_back(std::move(entry));
    }

    std::string extra;

    if (file >> extra) {
        error = "Unexpected data after saved cache entries.";
        return false;
    }

    const auto now = std::chrono::steady_clock::now();

    const auto maxRemaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::time_point::max() - now
        ).count();
    for (const auto& entry : entries) {
        if (entry.remainingTTL > maxRemaining) {
            error = "TTL value exceeds the supported time range.";
            return false;
        }
    }

    std::unordered_map<std::string, CacheEntry> restored;
    restored.reserve(std::min(entryCount, cache.capacity));

    CacheEntry* newHead = nullptr;
    CacheEntry* newTail = nullptr;

    for (std::size_t i = 0;
         i < entries.size() && i < cache.capacity;
         ++i) {

        const auto& saved = entries[i];

        CacheEntry restoredEntry;
        restoredEntry.value = saved.value;

        if (saved.remainingTTL != -1) {
            restoredEntry.expiresAt =
                now + std::chrono::milliseconds(saved.remainingTTL);
        }

        auto [it, inserted] = restored.emplace(
            saved.key,
            std::move(restoredEntry)
        );

        if (!inserted) {
            error = "Unable to restore cache entry.";
            return false;
        }

        CacheEntry* entry = &it->second;

        entry->key = &it->first;
        entry->previous = newTail;
        entry->next = nullptr;

        if (newTail != nullptr) {
            newTail->next = entry;
        } else {
            newHead = entry;
        }

        newTail = entry;
    }

    {
        std::lock_guard<std::mutex> lock(cache.mutex);

        cache.data.swap(restored);
        cache.head = newHead;
        cache.tail = newTail;
        cache.stats = CacheStats{};
    }

    error.clear();
    return true;
}
