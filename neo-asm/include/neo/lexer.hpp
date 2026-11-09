/**
 * @file lexer.hpp
 * @brief Token definitions and lexical analyzer interface for Neo-ASM.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace neo {

// System register alias IDs (distinct from virtual registers 0..31)
inline constexpr uint8_t REG_SP = 32;
inline constexpr uint8_t REG_FP = 33;
inline constexpr uint8_t REG_ZR = 34;
inline constexpr uint8_t REG_INVALID = 0xFF;

/**
 * @brief Categorizes lexical tokens produced by NeoLexer.
 */
enum class TokenType : uint8_t {
    Eof,
    Newline,
    Identifier,
    Register,
    Integer,
    String,
    Colon,
    Comma,
    LeftBracket,
    RightBracket,
    LeftParen,
    RightParen,
    Plus,
    Minus,
    Origin,
    Invalid
};

/**
 * @brief Source line and column coordinates for diagnostics.
 */
struct SourceLocation {
    uint32_t line{1};
    uint32_t column{1};
};

/**
 * @brief Token produced during lexical analysis.
 */
struct Token {
    TokenType type{TokenType::Eof};
    std::string text;
    int64_t int_value{0};
    uint8_t reg_index{REG_INVALID};
    bool is_system_reg{false};
    uint32_t indent{0};
    bool is_line_start{false};
    SourceLocation location;
};

/**
 * @brief Lexical analyzer tokenizing Neo-ASM (.neoa) source code.
 */
class NeoLexer {
public:
    explicit NeoLexer(std::string_view source) noexcept;

    /**
     * @brief Consumes and returns the next token in the stream.
     * @return Next token, or EOF token if end of stream reached.
     */
    Token next_token();

    /**
     * @brief Tokenizes the entire source buffer into a vector of tokens.
     * @return Contiguous list of tokens ending with EOF.
     */
    std::vector<Token> tokenize();

private:
    std::string_view source_;
    size_t cursor_{0};
    uint32_t line_{1};
    uint32_t column_{1};
    uint32_t current_line_indent_{0};
    bool is_at_line_start_{true};

    [[nodiscard]] bool is_at_end() const noexcept;
    [[nodiscard]] char peek() const noexcept;
    [[nodiscard]] char peek_next() const noexcept;
    char advance() noexcept;

    void skip_whitespace_and_comments();
    Token scan_identifier_or_register(SourceLocation loc);
    Token scan_origin(SourceLocation loc);
    Token scan_number(SourceLocation loc);
    Token scan_string(SourceLocation loc);
    Token scan_block_comment(SourceLocation loc);
};

} // namespace neo
