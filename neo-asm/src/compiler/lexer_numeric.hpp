/**
 * @file lexer_numeric.hpp
 * @brief Numeric literal parsing for NeoLexer.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace neo {

/**
 * @brief Parses an integer literal from assembly source text.
 *
 * Supports hexadecimal (0x/0X), binary (0b/0B), octal (0o/0O), and decimal,
 * including optional underscores (_) as digit separators and leading +/- signs.
 *
 * @param text String view containing numeric literal characters.
 * @return Parsed 64-bit integer, or std::nullopt if malformed.
 */
[[nodiscard]] std::optional<int64_t> parse_numeric_literal(std::string_view text) noexcept;

} // namespace neo
