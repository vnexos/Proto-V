/**
 * @file register_mapping.hpp
 * @brief Architecture-specific register translation routines.
 */

#pragma once

#include "neo/register_alloc.hpp"

namespace neo::detail {

/**
 * @brief Translates register index to AArch64 hardware ID and name.
 */
[[nodiscard]] RegisterMapping map_aarch64(uint8_t reg, bool is_system);

/**
 * @brief Translates register index to RISC-V 64 hardware ID and name.
 */
[[nodiscard]] RegisterMapping map_riscv64(uint8_t reg, bool is_system);

/**
 * @brief Translates register index to x86_64 hardware ID, pinned GPR, or spill slot.
 */
[[nodiscard]] RegisterMapping map_x86_64(uint8_t reg, bool is_system);

} // namespace neo::detail
