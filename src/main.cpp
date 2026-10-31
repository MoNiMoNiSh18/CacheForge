#include "cache/cache.h"
#include "cache/persistence.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <cctype>
#include <exception>
#include <limits>
#include <stdexcept>

void showHelp() {
    std::cout << "\n========== CacheForge CLI ==========\n"
              << "SET key value [ttl_seconds]  Insert or update a value\n"
              << "GET key                      Retrieve a value\n"
              << "DELETE key                   Remove a key\n"
              << "SIZE                         Display current cache size\n"
              << "CLEANUP                      Remove expired entries\n"
              << "STATS                        Display cache statistics\n"
              << "SAVE filename                Save cache contents to a file\n"
              << "LOAD filename                Load cache contents from a file\n"
              << "HELP                         Show available commands\n"
              << "EXIT                         Exit CacheForge\n"
              << "====================================\n\n";
}

bool hasExtraArgument(std::istringstream& input) {
    std::string extra;
    return static_cast<bool>(input >> extra);
}

int main(int argc, char* argv[]) {
    std::size_t capacity = 1000;

    if (argc > 2) {
        std::cerr << "Usage: cacheforge [capacity]\n";
        return 1;
    }

    if (argc == 2) {
        try {
            std::string capacityArg = argv[1];
            std::size_t parsed = 0;

            unsigned long long requested =
                std::stoull(capacityArg, &parsed);

            if (parsed != capacityArg.size() ||
                capacityArg.empty() ||
                capacityArg.front() == '-' ||
                requested == 0 ||
                requested > std::numeric_limits<std::size_t>::max()) {
                throw std::invalid_argument("Invalid capacity");
            }

            capacity = static_cast<std::size_t>(requested);
        }
        catch (const std::exception&) {
            std::cerr << "Error: capacity must be a positive integer.\n";
            return 1;
        }
    }

    Cache cache(capacity);

    std::cout << "====================================\n"
              << "       CacheForge V1.0.0 CLI\n"
              << "====================================\n"
              << "Capacity: " << capacity << " entries\n"
              << "Type HELP to see available commands.\n";
              
    std::string line;

    while (true) {
        std::cout << "\nCacheForge> ";

        if (!std::getline(std::cin, line)) {
            break;
        }

        std::istringstream input(line);
        std::string command;

        if (!(input >> command)) {
            continue;
        }

        std::transform(
            command.begin(),
            command.end(),
            command.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            }
        );

        if (command == "SET") {
            std::string key;
            std::string value;

            if (!(input >> key >> std::quoted(value))) {
                std::cout << "Usage: SET key value [ttl_seconds]\n";
                continue;
            }

            std::string ttlToken;
            long long ttlSeconds = 0;

            if (input >> ttlToken) {
                try {
                    std::size_t parsed = 0;
                    ttlSeconds = std::stoll(ttlToken, &parsed);

                    if (parsed != ttlToken.size() || ttlSeconds < 0) {
                        std::cout << "TTL must be a non-negative integer.\n";
                        continue;
                    }
                } catch (const std::exception&) {
                    std::cout << "Invalid TTL value.\n";
                    continue;
                }
            }

            if (hasExtraArgument(input)) {
                std::cout << "Too many arguments.\n";
                continue;
            }

            cache.set(key, value, std::chrono::seconds(ttlSeconds));
            std::cout << "Stored key: " << key << '\n';
        }

        else if (command == "GET") {
            std::string key;

            if (!(input >> key) || hasExtraArgument(input)) {
                std::cout << "Usage: GET key\n";
                continue;
            }

            auto value = cache.get(key);

            if (value.has_value()) {
                std::cout << "Value: " << *value << '\n';
            } else {
                std::cout << "Key not found or expired.\n";
            }
        }

        else if (command == "DELETE") {
            std::string key;

            if (!(input >> key) || hasExtraArgument(input)) {
                std::cout << "Usage: DELETE key\n";
                continue;
            }

            if (cache.remove(key)) {
                std::cout << "Key deleted successfully.\n";
            } else {
                std::cout << "Key not found.\n";
            }
        }

        else if (command == "SIZE") {
            if (hasExtraArgument(input)) {
                std::cout << "Usage: SIZE\n";
                continue;
            }

            std::cout << "Current cache size: " << cache.size() << '\n';
        }

        else if (command == "CLEANUP") {
            if (hasExtraArgument(input)) {
                std::cout << "Usage: CLEANUP\n";
                continue;
            }

            std::size_t removed = cache.cleanupExpired();
            std::cout << "Expired entries removed: " << removed << '\n';
        }

        else if (command == "STATS") {
            if (hasExtraArgument(input)) {
                std::cout << "Usage: STATS\n";
                continue;
            }

            CacheStats stats = cache.getStats();

            std::cout << "\n========== Cache Statistics ==========\n"
                      << "Hits:       " << stats.hits << '\n'
                      << "Misses:     " << stats.misses << '\n'
                      << "Insertions: " << stats.insertions << '\n'
                      << "Evictions:  " << stats.evictions << '\n'
                      << "Hit rate:   " << stats.hitRate() << "%\n"
                      << "======================================\n";
        }

                else if (command == "SAVE") {
            std::string filename;

            if (!(input >> std::quoted(filename)) || hasExtraArgument(input)) {
                std::cout << "Usage: SAVE filename\n";
                continue;
            }

            std::string error;

            if (CachePersistence::save(cache, filename, error)) {
                std::cout << "Cache saved successfully to: "
                        << filename << '\n';
            } else {
                std::cout << "Save failed: " << error << '\n';
            }
        }

        else if (command == "LOAD") {
                std::string filename;

                if (!(input >> std::quoted(filename)) || hasExtraArgument(input)) {
                    std::cout << "Usage: LOAD filename\n";
                    continue;
                }

                std::string error;

                if (CachePersistence::load(cache, filename, error)) {
                    std::cout << "Cache loaded successfully from: "
                            << filename << '\n'
                            << "Restored entries: " << cache.size() << '\n'
                            << "Statistics have been reset.\n";
                } else {
                    std::cout << "Load failed: " << error << '\n';
                }
            }

        else if (command == "HELP") {
            if (hasExtraArgument(input)) {
                std::cout << "Usage: HELP\n";
                continue;
            }

            showHelp();
        }

        else if (command == "EXIT") {
            if (hasExtraArgument(input)) {
                std::cout << "Usage: EXIT\n";
                continue;
            }

            std::cout << "Shutting down CacheForge. Goodbye!\n";
            break;
        }

        else {
            std::cout << "Unknown command. Type HELP for available commands.\n";
        }
    }

    return 0;
}