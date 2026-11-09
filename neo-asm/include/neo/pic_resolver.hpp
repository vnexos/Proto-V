/**
 * @file pic_resolver.hpp
 * @brief Position-Independent Code (PIC) branch displacement resolver interface.
 */

#pragma once

#include "neo/common.hpp"
#include "neo/parser.hpp"
#include "neo/schema.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace neo {

/**
 * @brief Categorizes failure conditions during PIC branch displacement resolution.
 */
enum class PicErrorCode : uint8_t {
    Success = 0,
    UndefinedLabel = 1,
    DisplacementOutOfRange = 2,
    InvalidAlignment = 3,
    UnsupportedArchitecture = 4,
    InvalidBitWidth = 5
};

/**
 * @brief Outcome of a relative branch displacement resolution attempt.
 */
struct PicDisplacementResult {
    bool success{false};
    int64_t displacement{0};
    uint64_t target_pc{0};
    PicErrorCode error_code{PicErrorCode::Success};
    std::string error_message;
};

/**
 * @brief Resolves PC-relative branch displacements for forward, backward, and zero jumps.
 *
 * Implements two-pass address tracking and displacement calculation without requiring
 * external relocation tables (.reloc). Enforces architecture-specific branch displacement
 * arithmetic and bitfield width bounds for x86_64, AArch64, and RISC-V 64.
 */
class NeoPicResolver {
public:
    NeoPicResolver() = default;

    /**
     * @brief Records the absolute byte position (PC) of a label tag.
     * @param label_name Identifier (simple, relative, or hierarchical dotted path).
     * @param pc Byte address (PC) corresponding to the label.
     */
    void record_label(std::string_view label_name, uint64_t pc);

    /**
     * @brief Sets the absolute byte position (PC) for an instruction by sequence index.
     * @param instruction_index Index of the instruction in the program sequence.
     * @param pc Byte address (PC) of the instruction.
     */
    void set_instruction_pc(size_t instruction_index, uint64_t pc);

    /**
     * @brief Scans instruction byte lengths and assigns contiguous PCs starting at base_pc.
     * @param instruction_lengths Byte lengths for each instruction in the program.
     * @param base_pc Initial starting program counter (default 0).
     */
    void scan_instruction_lengths(const std::vector<uint8_t>& instruction_lengths, uint64_t base_pc = 0);

    /**
     * @brief Imports labels from a SymbolTable, binding PCs via recorded instruction PCs.
     * @param symbols Parsed symbol table containing hierarchical label definitions.
     * @param total_bytes Total byte size of the program (used for labels at end of stream).
     */
    void import_symbol_table(const SymbolTable& symbols, uint64_t total_bytes = 0);

    /**
     * @brief Gets the recorded PC for an instruction index.
     * @param instruction_index Index of the instruction.
     * @return Instruction PC, or std::nullopt if unrecorded.
     */
    [[nodiscard]] std::optional<uint64_t> get_instruction_pc(size_t instruction_index) const noexcept;

    /**
     * @brief Resolves a label name considering scope hierarchy and returns its PC.
     * @param label_name Target label to search for.
     * @param current_scope Current dotted scope path (e.g. "label_a.label_b").
     * @return Target PC, or std::nullopt if label cannot be found.
     */
    [[nodiscard]] std::optional<uint64_t> get_label_pc(
        std::string_view label_name,
        std::string_view current_scope = "") const;

    /**
     * @brief Resolves the canonical full path name for a label within an active scope.
     * @param label_name Target label to search for.
     * @param current_scope Current dotted scope path.
     * @return Fully resolved label name, or empty string if not found.
     */
    [[nodiscard]] std::string resolve_label_name(
        std::string_view label_name,
        std::string_view current_scope = "") const;

    /**
     * @brief Checks if a label has been recorded in the symbol index.
     * @param label_name Fully qualified or recorded label name.
     * @return True if label is present.
     */
    [[nodiscard]] bool has_label(std::string_view label_name) const noexcept;

    /**
     * @brief Clears all recorded labels and instruction PCs.
     */
    void clear() noexcept;

    /**
     * @brief Pure calculation of PC-relative branch displacement with bounds and alignment checks.
     *
     * Formulas:
     * - x86_64: disp = target_pc - (current_pc + insn_length)
     * - AArch64: disp = (target_pc - current_pc) >> 2
     * - RISC-V 64: disp = target_pc - current_pc
     *
     * @param target_pc Destination target address.
     * @param current_pc Address of the branch instruction.
     * @param insn_length Length of the branch instruction in bytes.
     * @param bit_width Width in bits of the operand slot immediate/displacement field.
     * @param arch_id Target architecture ID (USX_ARCH_X86_64, USX_ARCH_AARCH64, USX_ARCH_RISCV64).
     * @return PicDisplacementResult containing computed displacement or descriptive error.
     */
    [[nodiscard]] static PicDisplacementResult calculate_displacement(
        uint64_t target_pc,
        uint64_t current_pc,
        uint8_t insn_length,
        uint8_t bit_width,
        uint16_t arch_id) noexcept;

    /**
     * @brief Resolves a label's address and computes the relative branch displacement.
     * @param target_label Branch target label identifier.
     * @param current_pc Address of the branch instruction.
     * @param insn_length Length of the branch instruction in bytes.
     * @param bit_width Width in bits of the operand slot.
     * @param arch_id Target architecture ID.
     * @param current_scope Current hierarchical dotted scope path.
     * @return PicDisplacementResult with status and displacement value.
     */
    [[nodiscard]] PicDisplacementResult resolve_displacement(
        std::string_view target_label,
        uint64_t current_pc,
        uint8_t insn_length,
        uint8_t bit_width,
        uint16_t arch_id,
        std::string_view current_scope = "") const;

    /**
     * @brief Resolves displacement using a NeoOperandSlot descriptor.
     */
    [[nodiscard]] PicDisplacementResult resolve_displacement(
        std::string_view target_label,
        uint64_t current_pc,
        uint8_t insn_length,
        const NeoOperandSlot& slot,
        uint16_t arch_id,
        std::string_view current_scope = "") const;

    /**
     * @brief Resolves displacement for a specific instruction index in the recorded sequence.
     */
    [[nodiscard]] PicDisplacementResult resolve_displacement_for_instruction(
        size_t instruction_index,
        std::string_view target_label,
        uint8_t insn_length,
        uint8_t bit_width,
        uint16_t arch_id,
        std::string_view current_scope = "") const;

private:
    std::unordered_map<std::string, uint64_t> tag_positions_;
    std::vector<uint64_t> instruction_pcs_;
};

} // namespace neo
