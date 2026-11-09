/**
 * @file operand_parser.cpp
 * @brief Operand parsing implementation for NeoParser.
 */

#include "operand_parser.hpp"

namespace neo {

namespace {

std::string format_loc(SourceLocation loc) {
    return " at line " + std::to_string(loc.line) + ", column " + std::to_string(loc.column);
}

} // namespace

std::optional<Operand> OperandParser::parse_memory(std::span<const Token> tokens,
                                                   size_t& pos,
                                                   std::string& out_error) {
    const SourceLocation open_loc = tokens[pos].location;
    pos++; // Consume '['

    if (pos >= tokens.size()) {
        out_error = "Unclosed memory bracket ']'" + format_loc(open_loc);
        return std::nullopt;
    }
    if (tokens[pos].type != TokenType::Register) {
        out_error = "Expected base register inside memory operand" + format_loc(tokens[pos].location);
        return std::nullopt;
    }
    if (tokens[pos].reg_index == REG_INVALID) {
        out_error = "Invalid base register '" + tokens[pos].text + "'" + format_loc(tokens[pos].location);
        return std::nullopt;
    }

    const uint8_t base_reg = tokens[pos].reg_index;
    const bool is_base_system = tokens[pos].is_system_reg;
    pos++;

    int64_t disp = 0;
    if (pos < tokens.size() && (tokens[pos].type == TokenType::Plus || tokens[pos].type == TokenType::Minus)) {
        const bool is_neg = (tokens[pos].type == TokenType::Minus);
        pos++;
        if (pos >= tokens.size() || tokens[pos].type != TokenType::Integer) {
            out_error = "Expected displacement value after sign" + format_loc(open_loc);
            return std::nullopt;
        }
        disp = is_neg ? -tokens[pos].int_value : tokens[pos].int_value;
        pos++;
    }

    if (pos >= tokens.size() || tokens[pos].type != TokenType::RightBracket) {
        out_error = "Unclosed memory bracket ']'" + format_loc(open_loc);
        return std::nullopt;
    }
    pos++; // Consume ']'

    Operand op;
    op.kind = OperandKind::Memory;
    op.mem = {base_reg, disp, is_base_system};
    op.location = open_loc;
    return op;
}

std::optional<Operand> OperandParser::parse_immediate(std::span<const Token> tokens,
                                                      size_t& pos,
                                                      std::string& out_error) {
    bool is_neg = false;
    const SourceLocation loc = tokens[pos].location;
    if (tokens[pos].type == TokenType::Plus || tokens[pos].type == TokenType::Minus) {
        is_neg = (tokens[pos].type == TokenType::Minus);
        pos++;
    }

    if (pos >= tokens.size() || tokens[pos].type != TokenType::Integer) {
        out_error = "Expected numeric integer literal" + format_loc(loc);
        return std::nullopt;
    }

    const int64_t val = is_neg ? -tokens[pos].int_value : tokens[pos].int_value;
    pos++;

    Operand op;
    op.kind = OperandKind::Immediate;
    op.immediate = val;
    op.location = loc;
    return op;
}

std::optional<Operand> OperandParser::parse_operand(std::span<const Token> tokens,
                                                    size_t& pos,
                                                    std::string& out_error) {
    if (pos >= tokens.size()) {
        out_error = "Unexpected end of operand stream";
        return std::nullopt;
    }

    const auto& tok = tokens[pos];
    if (tok.type == TokenType::LeftBracket) {
        return parse_memory(tokens, pos, out_error);
    }
    if (tok.type == TokenType::Integer || tok.type == TokenType::Plus || tok.type == TokenType::Minus) {
        return parse_immediate(tokens, pos, out_error);
    }
    if (tok.type == TokenType::Register) {
        if (tok.reg_index == REG_INVALID) {
            out_error = "Invalid register '" + tok.text + "'" + format_loc(tok.location);
            return std::nullopt;
        }
        pos++;
        return Operand{OperandKind::Register, tok.reg_index, tok.is_system_reg, 0, {}, "", "", tok.location};
    }
    if (tok.type == TokenType::Identifier) {
        pos++;
        return Operand{OperandKind::Label, REG_INVALID, false, 0, {}, tok.text, "", tok.location};
    }

    out_error = "Unexpected operand token '" + tok.text + "'" + format_loc(tok.location);
    return std::nullopt;
}

} // namespace neo
