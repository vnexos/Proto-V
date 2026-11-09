/**
 * @file parser.hpp
 * @brief Abstract Syntax Tree (AST) definitions and parser interface for Neo-ASM.
 */

#pragma once

#include "neo/lexer.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace neo {

/**
 * @brief Discriminator for AST operand types.
 */
enum class OperandKind : uint8_t {
    None = 0,
    Register = 1,
    Immediate = 2,
    Memory = 3,
    Label = 4
};

/**
 * @brief Memory operand representation [base + displacement].
 */
struct MemoryOperand {
    uint8_t base_reg{REG_INVALID};  ///< Base register ID (0..31 or system alias)
    int64_t displacement{0};        ///< Offset value added to base register
    bool is_base_system{false};     ///< True if base register is system alias (sp, fp, zr)
};

/**
 * @brief AST instruction operand descriptor.
 */
struct Operand {
    OperandKind kind{OperandKind::None};
    uint8_t reg{REG_INVALID};       ///< For OperandKind::Register
    bool is_system_reg{false};      ///< True if system register (sp, fp, zr)
    int64_t immediate{0};           ///< For OperandKind::Immediate
    MemoryOperand mem{};            ///< For OperandKind::Memory
    std::string label;              ///< For OperandKind::Label (source identifier)
    std::string resolved_label;     ///< Fully qualified resolved label path
    SourceLocation location;
};

/**
 * @brief Single parsed Neo-ASM instruction or native origin passthrough.
 */
struct AstInstruction {
    std::string mnemonic;           ///< Canonical mnemonic (e.g. "ASN", "ADD", "origin(SYSCALL)")
    std::vector<Operand> operands;  ///< Ordered list of instruction operands
    std::string origin_command;     ///< For origin(...) instructions: inner command string
    bool is_origin{false};          ///< True if native origin passthrough
    std::string scope;              ///< Enclosing hierarchical label scope
    SourceLocation location;
};

/**
 * @brief Information recorded for a defined label symbol.
 */
struct LabelSymbol {
    std::string name;               ///< Simple local name (e.g. "label_c")
    std::string full_path;          ///< Fully qualified path (e.g. "label_a.label_b.label_c")
    std::string parent_scope;       ///< Enclosing parent scope path
    uint32_t indent{0};             ///< Source indentation level
    size_t instruction_index{0};    ///< Target instruction index in parsed sequence
    SourceLocation location;
};

/**
 * @brief Hierarchical symbol table managing label scopes and lookups.
 */
class SymbolTable {
public:
    void add_label(const LabelSymbol& symbol);
    [[nodiscard]] const LabelSymbol* find_exact(std::string_view full_path) const noexcept;
    [[nodiscard]] std::string resolve_label(std::string_view target, std::string_view current_scope) const;
    [[nodiscard]] bool contains(std::string_view full_path) const noexcept;
    [[nodiscard]] const std::unordered_map<std::string, LabelSymbol>& labels() const noexcept;

private:
    std::unordered_map<std::string, LabelSymbol> labels_;
};

/**
 * @brief Result returned from parsing a Neo-ASM source file or buffer.
 */
struct ParseResult {
    bool success{false};
    std::vector<AstInstruction> instructions;
    SymbolTable symbols;
    std::vector<std::string> errors;
};

/**
 * @brief Recursive-descent parser constructing AST and hierarchical symbols from tokens.
 */
class NeoParser {
public:
    [[nodiscard]] static ParseResult parse(std::string_view source);
};

} // namespace neo
