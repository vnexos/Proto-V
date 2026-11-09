/**
 * @file emitter.hpp
 * @brief Architecture-aware machine code instruction emitter and bitfield injector.
 */

#pragma once

#include "neo/common.hpp"
#include "neo/schema.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

namespace neo {

/**
 * @brief Handles bitfield slot masking, shifting, and machine code byte emission.
 */
class NeoEmitter {
public:
    /**
     * @brief Checks whether an operand value fits within the operand slot's bitfield width.
     * @param slot The operand slot specification.
     * @param value The 64-bit operand value.
     * @param arch_id The target architecture ID.
     * @return True if value is representable without overflow, false otherwise.
     */
    [[nodiscard]] static bool is_value_in_bounds(
        const NeoOperandSlot& slot,
        int64_t value,
        uint16_t arch_id) noexcept;

    /**
     * @brief Dynamically injects an operand value into an instruction buffer bitfield slot.
     *
     * Handles architecture differences:
     * - AArch64 and RISC-V 64: 32-bit little-endian instruction word masking & bitfield shifting.
     * - x86_64: ModR/M byte-relative offsets and shifts, REX prefix extensions, and 32-bit displacements.
     *
     * Enforces bit-width bounds checking to prevent field overflow corruption.
     *
     * @param buffer Pointer to raw instruction byte buffer.
     * @param byte_length Length of instruction in bytes.
     * @param slot The operand slot definition (bit_offset, bit_width, mask, slot_type).
     * @param value Operand value to inject (register ID, immediate, displacement).
     * @param arch_id Target architecture ID (USX_ARCH_X86_64, USX_ARCH_AARCH64, USX_ARCH_RISCV64).
     * @return True if operand was successfully injected, false on bounds/length overflow.
     */
    static bool inject_operand(
        uint8_t* buffer,
        uint8_t byte_length,
        const NeoOperandSlot& slot,
        int64_t value,
        uint16_t arch_id) noexcept;

    /**
     * @brief Emits a single machine code instruction from a template and operand values.
     * Copies opcode template bytes, injects active operand values, and appends to out_buffer.
     *
     * @param tmpl Instruction template containing raw opcode bytes and operand slots.
     * @param operand_values Span of operand values corresponding to active template slots.
     * @param arch_id Target architecture ID.
     * @param out_buffer Byte vector where emitted machine code bytes will be appended.
     * @return True if emission succeeded, false if any operand injection failed.
     */
    static bool emit_instruction(
        const NeoInstructionTemplate& tmpl,
        std::span<const int64_t> operand_values,
        uint16_t arch_id,
        std::vector<uint8_t>& out_buffer);

    /**
     * @brief Emits a single machine code instruction and returns it as a byte vector.
     *
     * @param tmpl Instruction template.
     * @param operand_values Span of operand values.
     * @param arch_id Target architecture ID.
     * @return Emitted machine code byte vector (empty on error).
     */
    [[nodiscard]] static std::vector<uint8_t> emit_instruction(
        const NeoInstructionTemplate& tmpl,
        std::span<const int64_t> operand_values,
        uint16_t arch_id);

    /**
     * @brief Overload of emit_instruction taking an initializer list of operand values.
     */
    static bool emit_instruction(
        const NeoInstructionTemplate& tmpl,
        std::initializer_list<int64_t> operand_values,
        uint16_t arch_id,
        std::vector<uint8_t>& out_buffer);

    /**
     * @brief Overload of emit_instruction returning byte vector from an initializer list.
     */
    [[nodiscard]] static std::vector<uint8_t> emit_instruction(
        const NeoInstructionTemplate& tmpl,
        std::initializer_list<int64_t> operand_values,
        uint16_t arch_id);
};

} // namespace neo
