/**
 * @file lexer_registers.cpp
 * @brief Register matching implementation for NeoLexer.
 */

#include "lexer_registers.hpp"
#include "neo/lexer.hpp"

#include <cctype>
#include <charconv>

namespace neo {

bool equals_ci(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

bool match_register(std::string_view text, uint8_t& out_reg, bool& out_system) noexcept {
    if (equals_ci(text, "sp")) { out_reg = REG_SP; out_system = true; return true; }
    if (equals_ci(text, "fp")) { out_reg = REG_FP; out_system = true; return true; }
    if (equals_ci(text, "zr")) { out_reg = REG_ZR; out_system = true; return true; }

    if (text.size() >= 2 && (text[0] == 'r' || text[0] == 'R')) {
        unsigned int idx = 0;
        const auto* start = text.data() + 1;
        const auto* end = text.data() + text.size();
        const auto res = std::from_chars(start, end, idx);
        if (res.ec == std::errc{} && res.ptr == end) {
            out_reg = (idx <= 31) ? static_cast<uint8_t>(idx) : REG_INVALID;
            out_system = false;
            return true;
        }
    }
    return false;
}

} // namespace neo
