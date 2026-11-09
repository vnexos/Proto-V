/**
 * @file lexer.cpp
 * @brief Lexical analyzer implementation for Neo-ASM source code.
 */

#include "neo/lexer.hpp"
#include "lexer_numeric.hpp"
#include "lexer_registers.hpp"

#include <cctype>

namespace neo {

NeoLexer::NeoLexer(std::string_view source) noexcept : source_(source) {}

bool NeoLexer::is_at_end() const noexcept { return cursor_ >= source_.size(); }
char NeoLexer::peek() const noexcept { return is_at_end() ? '\0' : source_[cursor_]; }
char NeoLexer::peek_next() const noexcept {
    return (cursor_ + 1 >= source_.size()) ? '\0' : source_[cursor_ + 1];
}

char NeoLexer::advance() noexcept {
    const char c = source_[cursor_++];
    column_++;
    return c;
}

void NeoLexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        const char c = peek();
        if (c == ' ') {
            if (is_at_line_start_) current_line_indent_++;
            advance();
        } else if (c == '\t') {
            if (is_at_line_start_) current_line_indent_ = ((current_line_indent_ / 4) + 1) * 4;
            advance();
        } else if (c == '\r') {
            advance();
        } else if (c == ';') {
            while (!is_at_end() && peek() != '\n') advance();
        } else if (c == '/' && peek_next() == '*') {
            advance(); advance();
            while (!is_at_end() && !(peek() == '*' && peek_next() == '/')) {
                if (peek() == '\n') { line_++; column_ = 0; }
                advance();
            }
            if (!is_at_end()) { advance(); advance(); }
        } else {
            break;
        }
    }
}

Token NeoLexer::scan_origin(SourceLocation loc) {
    while (!is_at_end() && peek() != '(') advance();
    if (is_at_end()) return {TokenType::Invalid, "Malformed origin", 0, REG_INVALID, false, 0, false, loc};
    advance(); // Consume '('

    size_t start = cursor_;
    while (!is_at_end() && peek() != ')' && peek() != '\n') advance();
    if (is_at_end() || peek() != ')') {
        return {TokenType::Invalid, "Unclosed origin command", 0, REG_INVALID, false, 0, false, loc};
    }
    std::string_view inner = source_.substr(start, cursor_ - start);
    advance(); // Consume ')'

    while (!inner.empty() && std::isspace(static_cast<unsigned char>(inner.front()))) inner.remove_prefix(1);
    while (!inner.empty() && std::isspace(static_cast<unsigned char>(inner.back()))) inner.remove_suffix(1);
    return {TokenType::Origin, "origin(" + std::string(inner) + ")", 0, REG_INVALID, false, 0, false, loc};
}

Token NeoLexer::scan_identifier_or_register(SourceLocation loc) {
    const size_t start = cursor_;
    while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) ||
           peek() == '_' || peek() == '.' || peek() == '@')) {
        advance();
    }
    const std::string_view word = source_.substr(start, cursor_ - start);
    if (equals_ci(word, "origin") && peek() == '(') {
        cursor_ = start; column_ = loc.column;
        return scan_origin(loc);
    }
    uint8_t reg_idx = REG_INVALID;
    bool is_system = false;
    if (match_register(word, reg_idx, is_system)) {
        return {TokenType::Register, std::string(word), 0, reg_idx, is_system, 0, false, loc};
    }
    return {TokenType::Identifier, std::string(word), 0, REG_INVALID, false, 0, false, loc};
}

Token NeoLexer::scan_number(SourceLocation loc) {
    const size_t start = cursor_;
    if (peek() == '+' || peek() == '-') advance();
    while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }
    const std::string_view num_str = source_.substr(start, cursor_ - start);
    const auto val = parse_numeric_literal(num_str);
    if (!val) {
        return {TokenType::Invalid, "Invalid numeric literal: " + std::string(num_str), 0, REG_INVALID, false, 0, false, loc};
    }
    return {TokenType::Integer, std::string(num_str), *val, REG_INVALID, false, 0, false, loc};
}

Token NeoLexer::scan_string(SourceLocation loc) {
    const char quote = advance();
    const size_t start = cursor_;
    while (!is_at_end() && peek() != quote && peek() != '\n') advance();
    if (is_at_end() || peek() != quote) {
        return {TokenType::Invalid, "Unclosed string literal", 0, REG_INVALID, false, 0, false, loc};
    }
    const std::string val(source_.substr(start, cursor_ - start));
    advance(); // Consume quote
    return {TokenType::String, val, 0, REG_INVALID, false, 0, false, loc};
}

Token NeoLexer::next_token() {
    skip_whitespace_and_comments();
    const SourceLocation loc{line_, column_};
    const uint32_t indent = current_line_indent_;
    const bool at_line_start = is_at_line_start_;

    if (is_at_end()) return {TokenType::Eof, "", 0, REG_INVALID, false, indent, at_line_start, loc};
    is_at_line_start_ = false;
    const char c = peek();

    if (c == '\n') {
        advance(); line_++; column_ = 1; is_at_line_start_ = true; current_line_indent_ = 0;
        return {TokenType::Newline, "\n", 0, REG_INVALID, false, indent, at_line_start, loc};
    }
    if (c == ':') { advance(); return {TokenType::Colon, ":", 0, REG_INVALID, false, indent, at_line_start, loc}; }
    if (c == ',') { advance(); return {TokenType::Comma, ",", 0, REG_INVALID, false, indent, at_line_start, loc}; }
    if (c == '[') { advance(); return {TokenType::LeftBracket, "[", 0, REG_INVALID, false, indent, at_line_start, loc}; }
    if (c == ']') { advance(); return {TokenType::RightBracket, "]", 0, REG_INVALID, false, indent, at_line_start, loc}; }
    if (c == '(') { advance(); return {TokenType::LeftParen, "(", 0, REG_INVALID, false, indent, at_line_start, loc}; }
    if (c == ')') { advance(); return {TokenType::RightParen, ")", 0, REG_INVALID, false, indent, at_line_start, loc}; }
    if (c == '+') { advance(); return {TokenType::Plus, "+", 0, REG_INVALID, false, indent, at_line_start, loc}; }
    if (c == '-') { advance(); return {TokenType::Minus, "-", 0, REG_INVALID, false, indent, at_line_start, loc}; }

    if (c == '#' || (c == '/' && peek_next() == '/')) {
        advance();
        return {TokenType::Invalid, "Unsupported comment syntax (use ';' or '/* */')", 0, REG_INVALID, false, indent, at_line_start, loc};
    }
    if (std::isdigit(static_cast<unsigned char>(c))) return scan_number(loc);
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '.' || c == '@') {
        auto tok = scan_identifier_or_register(loc);
        tok.indent = indent; tok.is_line_start = at_line_start;
        return tok;
    }
    if (c == '"' || c == '\'') return scan_string(loc);

    advance();
    return {TokenType::Invalid, std::string("Unexpected character: ") + c, 0, REG_INVALID, false, indent, at_line_start, loc};
}

std::vector<Token> NeoLexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = next_token();
        tokens.push_back(tok);
        if (tok.type == TokenType::Eof) break;
    }
    return tokens;
}

} // namespace neo
