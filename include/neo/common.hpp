/**
 * @file common.hpp
 * @brief Common types, architecture IDs, and endianness utilities for Neo-ASM.
 */

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

namespace neo {

// Universal Secured Executable (USX) Target Architecture Constants
// Matches Code/Apeiron/shared/usx.h registry definitions
inline constexpr uint16_t USX_ARCH_X86_64  = 0x0001;
inline constexpr uint16_t USX_ARCH_AARCH64 = 0x0002;
inline constexpr uint16_t USX_ARCH_RISCV64 = 0x0004;

// Architecture Flags
// Bit 0: 0 = Little-Endian, 1 = Big-Endian
inline constexpr uint16_t USX_AFLAG_BIG_ENDIAN = 0x0001;
// Bit 1: 0 = 16 native GPRs (x86_64), 1 = 32 native GPRs (AArch64, RISC-V 64)
inline constexpr uint16_t USX_AFLAG_REG_COUNT  = 0x0002;

/**
 * @brief Checks if host machine is little-endian.
 * @return True if host byte order is little-endian, false otherwise.
 */
[[nodiscard]] constexpr bool is_little_endian() noexcept {
    return std::endian::native == std::endian::little;
}

/**
 * @brief Converts a 16-bit integer to little-endian byte order.
 * @param value Native 16-bit unsigned integer.
 * @return Value formatted in little-endian byte order.
 */
[[nodiscard]] constexpr uint16_t to_little_endian16(uint16_t value) noexcept {
    if constexpr (std::endian::native == std::endian::little) {
        return value;
    } else {
        return static_cast<uint16_t>((value >> 8) | (value << 8));
    }
}

/**
 * @brief Converts a 32-bit integer to little-endian byte order.
 * @param value Native 32-bit unsigned integer.
 * @return Value formatted in little-endian byte order.
 */
[[nodiscard]] constexpr uint32_t to_little_endian32(uint32_t value) noexcept {
    if constexpr (std::endian::native == std::endian::little) {
        return value;
    } else {
        return ((value & 0x000000FFu) << 24) |
               ((value & 0x0000FF00u) << 8)  |
               ((value & 0x00FF0000u) >> 8)  |
               ((value & 0xFF000000u) >> 24);
    }
}

/**
 * @brief Converts a 64-bit integer to little-endian byte order.
 * @param value Native 64-bit unsigned integer.
 * @return Value formatted in little-endian byte order.
 */
[[nodiscard]] constexpr uint64_t to_little_endian64(uint64_t value) noexcept {
    if constexpr (std::endian::native == std::endian::little) {
        return value;
    } else {
        uint64_t v = value;
        v = ((v & 0x00000000FFFFFFFFull) << 32) | ((v & 0xFFFFFFFF00000000ull) >> 32);
        v = ((v & 0x0000FFFF0000FFFFull) << 16) | ((v & 0xFFFF0000FFFF0000ull) >> 16);
        v = ((v & 0x00FF00FF00FF00FFull) << 8)  | ((v & 0xFF00FF00FF00FF00ull) >> 8);
        return v;
    }
}

} // namespace neo
