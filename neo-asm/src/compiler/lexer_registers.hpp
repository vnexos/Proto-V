/**
 * @file lexer_registers.hpp
 * @brief Register alias and identifier matching for NeoLexer.
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace neo {

/**
 * @brief Case-insensitively checks if two strings match.
 */
[[nodiscard]] bool equals_ci(std::string_view a, std::string_view b) noexcept;

/**
 * @brief Tests if text matches a virtual register (r0..r31) or system alias (sp/fp/zr).
 * @param text Candidate register string.
 * @param out_reg Output register index or REG_INVALID if > 31.
 * @param out_system Output flag set true for sp, fp, zr.
 * @return True if text follows register syntax, false otherwise.
 */
[[nodiscard]] bool match_register(std::string_view text, uint8_t& out_reg, bool& out_system) noexcept;

} // namespace neo
