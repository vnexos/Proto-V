/**
 * @file lexer_numeric.cpp
 * @brief Numeric literal parsing implementation for NeoLexer.
 */

#include "lexer_numeric.hpp"

#include <charconv>
#include <string>

namespace neo {

namespace {

/**
 * @brief Strips underscore separators from numeric literal text.
 */
std::string strip_separators(std::string_view text) {
    std::string cleaned;
    cleaned.reserve(text.size());
    for (char c : text) {
        if (c != '_') {
            cleaned.push_back(c);
        }
    }
    return cleaned;
}

/**
 * @brief Resolves base and numeric slice without prefix.
 */
std::pair<int, std::string_view> detect_radix_and_body(std::string_view cleaned) noexcept {
    if (cleaned.size() >= 2 && cleaned[0] == '0') {
        const char prefix = cleaned[1];
        if (prefix == 'x' || prefix == 'X') {
            return {16, cleaned.substr(2)};
        }
        if (prefix == 'b' || prefix == 'B') {
            return {2, cleaned.substr(2)};
        }
        if (prefix == 'o' || prefix == 'O') {
            return {8, cleaned.substr(2)};
        }
    }
    return {10, cleaned};
}

} // namespace

std::optional<int64_t> parse_numeric_literal(std::string_view text) noexcept {
    if (text.empty()) {
        return std::nullopt;
    }

    bool is_negative = false;
    if (text.front() == '-') {
        is_negative = true;
        text.remove_prefix(1);
    } else if (text.front() == '+') {
        text.remove_prefix(1);
    }

    if (text.empty()) {
        return std::nullopt;
    }

    const std::string cleaned = strip_separators(text);
    const auto [base, body] = detect_radix_and_body(cleaned);
    if (body.empty()) {
        return std::nullopt;
    }

    uint64_t unsigned_val = 0;
    const auto res = std::from_chars(body.data(), body.data() + body.size(), unsigned_val, base);
    if (res.ec != std::errc{} || res.ptr != body.data() + body.size()) {
        return std::nullopt;
    }

    if (is_negative) {
        return -static_cast<int64_t>(unsigned_val);
    }
    return static_cast<int64_t>(unsigned_val);
}

} // namespace neo
