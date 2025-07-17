#pragma once
#include <string_view>
#include <cstdint>
#include <cstring>

namespace HashUtils {

inline uint64_t fnv1a_hash(const char* data, size_t len) {
    const uint64_t FNV_OFFSET = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;

    uint64_t hash = FNV_OFFSET;
    for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint8_t>(data[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}

inline uint64_t hash_combine(uint64_t a, uint64_t b) {
    return a ^ (b + 0x9e3779b97f4a7c15 + (a << 12) + (a >> 4));
}

} // namespace HashUtils
