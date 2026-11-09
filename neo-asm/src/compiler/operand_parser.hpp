/**
 * @file operand_parser.hpp
 * @brief Operand parsing logic for NeoParser.
 */

#pragma once

#include "neo/lexer.hpp"
#include "neo/parser.hpp"

#include <optional>
#include <string>
#include <span>

namespace neo {

/**
 * @brief Parses an individual instruction operand from a token stream.
 */
class OperandParser {
public:
    /**
     * @brief Parses the next operand starting at index pos.
     * @param tokens Span of tokens for the current statement.
     * @param pos In/out position index within tokens.
     * @param out_error Output error message if parsing fails.
     * @return Parsed Operand, or std::nullopt on syntax error.
     */
    static std::optional<Operand> parse_operand(std::span<const Token> tokens,
                                                size_t& pos,
                                                std::string& out_error);

private:
    static std::optional<Operand> parse_memory(std::span<const Token> tokens,
                                               size_t& pos,
                                               std::string& out_error);

    static std::optional<Operand> parse_immediate(std::span<const Token> tokens,
                                                  size_t& pos,
                                                  std::string& out_error);
};

} // namespace neo
