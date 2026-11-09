/**
 * @file register_alloc.hpp
 * @brief Register virtualization and x86_64 stack spill sequencer interface.
 */

#pragma once

#include "neo/common.hpp"
#include "neo/lexer.hpp"
#include "neo/parser.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace neo {

/**
 * @brief Kind of operand in an expanded native instruction.
 */
enum class ExpandedOperandKind : uint8_t {
    Register = 1,
    Immediate = 2,
    Memory = 3,
    Label = 4
};

/**
 * @brief Memory reference operand [base + offset].
 */
struct ExpandedMemoryRef {
    uint8_t base_reg{REG_INVALID};  ///< Hardware base register ID
    int64_t offset{0};              ///< Immediate displacement in bytes
    std::string base_name;          ///< Architecture display name (e.g. "rbp", "sp")
};

/**
 * @brief Resolved and mapped operand for native instruction emission.
 */
struct ExpandedOperand {
    ExpandedOperandKind kind{ExpandedOperandKind::Register};
    uint8_t reg{REG_INVALID};       ///< Native hardware register ID
    std::string reg_name;           ///< Architecture register display name (e.g. "RAX", "X20")
    int64_t immediate{0};           ///< Immediate integer value
    ExpandedMemoryRef mem{};        ///< Memory operand details
    std::string label;              ///< Target branch label path
};

/**
 * @brief Hardware-mapped or scratch-expanded instruction ready for PIC/emission.
 */
struct ExpandedInstruction {
    std::string mnemonic;                  ///< Native or intermediate mnemonic
    std::vector<ExpandedOperand> operands; ///< Mapped native operands
    bool is_spill_sequence{false};         ///< True if generated as part of stack spill/scratch
    SourceLocation location{};             ///< Inherited source location

    /**
     * @brief Formats the instruction as human-readable assembly text.
     * @return Formatted assembly string (e.g. "ADD R15, RAX, RCX", "MOV RAX, [rbp - 8]").
     */
    [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Result of translating a virtual register to hardware or stack location.
 */
struct RegisterMapping {
    uint8_t hardware_id{REG_INVALID}; ///< Hardware register encoding ID
    bool is_spilled{false};           ///< True if spilled to stack frame [rbp - offset]
    int32_t spill_offset{0};          ///< Positive byte offset from frame pointer: (r - 14 + 1) * 8
    std::string name;                 ///< Architecture register name (e.g. "RAX", "X20", "x20")
};

/**
 * @brief Virtualizes 32 registers across architectures and synthesizes x86_64 spill sequences.
 */
class NeoRegisterAllocator {
public:
    /**
     * @brief Translates architecture name to standard USX architecture ID.
     * @param arch Architecture name string (e.g. "x86_64", "aarch64", "riscv64").
     * @return Canonical USX_ARCH_* identifier, or 0 if unknown.
     */
    [[nodiscard]] static uint16_t resolve_arch_id(std::string_view arch) noexcept;

    /**
     * @brief Computes stack frame spill offset for virtual registers r14..r31 on x86_64.
     * Formula: (r - 14 + 1) * 8
     * @param reg Virtual register index (14..31).
     * @return Positive byte offset relative to RBP (e.g. 8 for r14, 144 for r31).
     */
    [[nodiscard]] static constexpr int32_t spill_offset(uint8_t reg) noexcept {
        if (reg < 14 || reg > 31) return 0;
        return static_cast<int32_t>((reg - 14 + 1) * 8);
    }

    /**
     * @brief Checks if a virtual register requires stack spilling on the target architecture.
     * @param reg Virtual register index (0..31 or system register alias).
     * @param arch_id Target architecture ID.
     * @return True if register spills to stack (r14..r31 on x86_64).
     */
    [[nodiscard]] static bool is_spilled(uint8_t reg, uint16_t arch_id) noexcept;

    /**
     * @brief Maps a virtual register or system alias to native hardware ID and name.
     * @param reg Virtual register index (0..31 or REG_SP/REG_FP/REG_ZR).
     * @param is_system True if register is a system register alias.
     * @param arch_id Target architecture ID.
     * @return RegisterMapping containing hardware ID, spill status, and register name.
     */
    [[nodiscard]] static RegisterMapping map_register(uint8_t reg, bool is_system, uint16_t arch_id);

    /**
     * @brief Expands a single AST instruction into native or scratch-spilled instructions.
     * @param ast Parsed AST instruction.
     * @param arch_id Target architecture ID.
     * @return Vector of expanded instructions.
     */
    [[nodiscard]] static std::vector<ExpandedInstruction> expand_instruction(
        const AstInstruction& ast, uint16_t arch_id);

    /**
     * @brief Expands an entire program of AST instructions.
     * @param instructions List of parsed AST instructions.
     * @param arch_id Target architecture ID.
     * @return Vector of expanded instructions.
     */
    [[nodiscard]] static std::vector<ExpandedInstruction> expand_program(
        const std::vector<AstInstruction>& instructions, uint16_t arch_id);

    /**
     * @brief Parses assembly source and expands all instructions for the target architecture.
     * @param source Neo-ASM assembly source text.
     * @param arch_id Target architecture ID.
     * @return Vector of expanded instructions, empty if parsing fails.
     */
    [[nodiscard]] static std::vector<ExpandedInstruction> expand_source(
        std::string_view source, uint16_t arch_id);
};

} // namespace neo
