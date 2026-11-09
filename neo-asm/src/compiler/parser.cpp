/**
 * @file parser.cpp
 * @brief Recursive-descent AST parser implementation for Neo-ASM.
 */

#include "neo/parser.hpp"
#include "label_tracker.hpp"
#include "operand_parser.hpp"

#include <cctype>

namespace neo {

namespace {

std::string to_upper_str(std::string_view text) {
    std::string upper;
    upper.reserve(text.size());
    for (char c : text) {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return upper;
}

std::string format_error(std::string_view msg, SourceLocation loc) {
    return std::string(msg) + " at line " + std::to_string(loc.line) +
           ", column " + std::to_string(loc.column);
}

void skip_to_line_end(std::span<const Token> tokens, size_t& pos) noexcept {
    while (pos < tokens.size() && tokens[pos].type != TokenType::Newline &&
           tokens[pos].type != TokenType::Eof) {
        pos++;
    }
}

bool parse_instruction_operands(std::span<const Token> tokens,
                                size_t& pos,
                                AstInstruction& inst,
                                std::vector<std::string>& errors) {
    while (pos < tokens.size() && tokens[pos].type != TokenType::Newline &&
           tokens[pos].type != TokenType::Eof) {
        if (!inst.operands.empty()) {
            if (tokens[pos].type != TokenType::Comma) {
                errors.push_back(format_error("Expected ',' between operands", tokens[pos].location));
                skip_to_line_end(tokens, pos);
                return false;
            }
            pos++; // Consume ','
            if (pos >= tokens.size() || tokens[pos].type == TokenType::Newline ||
                tokens[pos].type == TokenType::Eof) {
                errors.push_back(format_error("Unexpected trailing comma", tokens[pos - 1].location));
                return false;
            }
        }
        std::string op_err;
        auto op = OperandParser::parse_operand(tokens, pos, op_err);
        if (!op) {
            errors.push_back(op_err);
            skip_to_line_end(tokens, pos);
            return false;
        }
        inst.operands.push_back(*op);
    }
    return true;
}

} // namespace

ParseResult NeoParser::parse(std::string_view source) {
    NeoLexer lexer(source);
    const auto tokens = lexer.tokenize();

    ParseResult result;
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::Invalid) {
            result.errors.push_back(format_error(tok.text, tok.location));
        }
    }
    if (!result.errors.empty()) {
        result.success = false;
        return result;
    }

    size_t pos = 0;
    LabelTracker label_tracker;

    while (pos < tokens.size()) {
        while (pos < tokens.size() && tokens[pos].type == TokenType::Newline) pos++;
        if (pos >= tokens.size() || tokens[pos].type == TokenType::Eof) break;

        // Label definition check
        if (tokens[pos].type == TokenType::Identifier && (pos + 1 < tokens.size()) &&
            tokens[pos + 1].type == TokenType::Colon) {
            label_tracker.push_label(tokens[pos].text, tokens[pos].indent, tokens[pos].location, result.symbols);
            pos += 2;
            if (pos < tokens.size() && tokens[pos].type == TokenType::Newline) { pos++; continue; }
            if (pos >= tokens.size() || tokens[pos].type == TokenType::Eof) break;
            if (tokens[pos].type == TokenType::Identifier && (pos + 1 < tokens.size()) &&
                tokens[pos + 1].type == TokenType::Colon) continue;
        }

        // Directives
        if (tokens[pos].type == TokenType::Identifier && tokens[pos].text.starts_with('.')) {
            skip_to_line_end(tokens, pos);
            continue;
        }

        if (tokens[pos].type != TokenType::Identifier && tokens[pos].type != TokenType::Origin) {
            result.errors.push_back(format_error("Expected instruction mnemonic", tokens[pos].location));
            skip_to_line_end(tokens, pos);
            continue;
        }

        AstInstruction inst;
        inst.location = tokens[pos].location;
        inst.scope = label_tracker.current_scope();

        if (tokens[pos].type == TokenType::Origin) {
            inst.is_origin = true;
            inst.mnemonic = tokens[pos].text;
            const auto open_p = inst.mnemonic.find('(');
            const auto close_p = inst.mnemonic.rfind(')');
            if (open_p != std::string::npos && close_p != std::string::npos && close_p > open_p) {
                inst.origin_command = inst.mnemonic.substr(open_p + 1, close_p - open_p - 1);
            }
            pos++;
        } else {
            inst.mnemonic = to_upper_str(tokens[pos].text);
            pos++;
        }

        if (parse_instruction_operands(tokens, pos, inst, result.errors)) {
            label_tracker.bind_pending_labels(result.instructions.size(), result.symbols);
            result.instructions.push_back(inst);
        }
    }

    label_tracker.finalize(result.instructions.size(), result.symbols);

    // Resolve hierarchical label targets
    for (auto& inst : result.instructions) {
        for (auto& op : inst.operands) {
            if (op.kind == OperandKind::Label) {
                op.resolved_label = result.symbols.resolve_label(op.label, inst.scope);
            }
        }
    }

    result.success = result.errors.empty();
    return result;
}

} // namespace neo
