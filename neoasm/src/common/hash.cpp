/**
 * @file hash.cpp
 * @brief 32-bit FNV-1a mnemonic name hashing utilities.
 */

#include <neo/common.hpp>

namespace neo {

/**
 * @brief Computes 32-bit FNV-1a hash over a byte span.
 *
 * Follows FNV-1a 32-bit specification with prime 0x01000193
 * and offset basis 0x811c9dc5.
 *
 * @param bytes Span of const bytes to hash.
 * @return 32-bit FNV-1a hash value.
 */
uint32_t fnv1a_32(std::span<const uint8_t> bytes) noexcept {
    uint32_t hash = 0x811c9dc5u;
    for (uint8_t b : bytes) {
        hash = (hash ^ b) * 0x01000193u;
    }
    return hash;
}

} // namespace neo
