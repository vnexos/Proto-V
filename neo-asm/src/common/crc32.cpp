/**
 * @file crc32.cpp
 * @brief IEEE 802.3 CRC32 checksum calculation implementation.
 */

#include <neo/common.hpp>

#include <array>

namespace neo {

namespace {

/**
 * @brief Generates the 256-entry lookup table for IEEE 802.3 CRC32 at compile-time.
 *
 * Polynomial 0xEDB88320 represents the reversed representation of the
 * standard Ethernet polynomial 0x04C11DB7.
 *
 * @return Compile-time 256-entry uint32_t table.
 */
constexpr auto generateCRC32Table() noexcept {
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }
    return table;
}

// Precomputed lookup table initialized at compile time
inline constexpr auto CRC32_TABLE = generateCRC32Table();

} // anonymous namespace

uint32_t calcCRC32(std::span<const uint8_t> data) noexcept {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint8_t byte : data) {
        crc = CRC32_TABLE[(crc ^ byte) & 0xFFu] ^ (crc >> 8);
    }
    return (crc ^ 0xFFFFFFFFu) & 0xFFFFFFFFu;
}

} // namespace neo
