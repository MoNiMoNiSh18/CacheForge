#include <iostream>
#include "cache/cache.h"

int main() {
    Cache cache;

    cache.set("user:101", "Monish");

    bool deleted = cache.remove("user:101");

    if (deleted) {
        std::cout << "Key deleted successfully\n";
    } else {
        std::cout << "Key not found\n";
    }

    auto value = cache.get("user:101");

    if (value.has_value()) {
        std::cout << "Value: " << value.value() << '\n';
    } else {
        std::cout << "Key not found\n";
    }

    bool deletedAgain = cache.remove("user:999");

    std::cout << "Delete missing key: "
              << (deletedAgain ? "Success" : "Key not found")
              << '\n';

    return 0;
}