#pragma once

#include <string>

#include "cache/cache.h"

class CachePersistence {
public:
    static bool save(
        const Cache& cache,
        const std::string& filename,
        std::string& error
    );

    static bool load(
        Cache& cache,
        const std::string& filename,
        std::string& error
    );
};