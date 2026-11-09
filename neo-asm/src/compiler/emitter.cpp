/**
 * @file emitter.cpp
 * @brief Machine code instruction emitter and bitfield injection implementation.
 */

#include "neo/emitter.hpp"

#include <algorithm>
#include <cstring>

namespace neo {

namespace {

/**
 * @brief Verifies register ID against architecture-specific bounds.
 */
bool check_register_bounds(int64_t value, uint8_t bit_width, uint16_t arch_id) noexcept {
    if (value < 0) return false;
    // x86_64 supports 16 hardware GPRs (0..15) with REX prefix extension
    if (arch_id == USX_ARCH_X86_64) return value <= 15;
    const uint64_t max_reg = (bit_width >= 64) ? ~0ULL : ((1ULL << bit_width) - 1);
    return static_cast<uint64_t>(value) <= max_reg;
}

/**
 * @brief Verifies signed integer value against bit-width capacity.
 */
bool check_signed_bounds(int64_t value, uint8_t bit_width) noexcept {
    if (bit_width >= 64) return true;
    const int64_t min_val = -(static_cast<int64_t>(1ULL << (bit_width - 1)));
    const int64_t max_val = static_cast<int64_t>((1ULL << (bit_width - 1)) - 1);
    return value >= min_val && value <= max_val;
}

/**
 * @brief Verifies immediate integer against signed or unsigned field capacity.
 */
bool check_immediate_bounds(int64_t value, uint8_t bit_width) noexcept {
    if (bit_width >= 64) return true;
    if (check_signed_bounds(value, bit_width)) return true;
    return value >= 0 && static_cast<uint64_t>(value) <= ((1ULL << bit_width) - 1);
}

/**
 * @brief Injects operand into 32-bit fixed instruction word (AArch64 / RISC-V 64).
 */
bool inject_aarch64_riscv(uint8_t* buffer, uint8_t byte_length, const NeoOperandSlot& slot, int64_t value) noexcept {
    if (byte_length < sizeof(uint32_t)) return false;

    // Fixed 32-bit instruction words are stored in little-endian byte order
    uint32_t word = 0;
    std::memcpy(&word, buffer, sizeof(uint32_t));
    word = to_little_endian32(word);

    const uint32_t mask = (slot.mask != 0)
        ? slot.mask
        : (((slot.bit_width >= 32) ? 0xFFFFFFFFu : ((1u << slot.bit_width) - 1u)) << slot.bit_offset);

    const uint32_t val32 = static_cast<uint32_t>(value);
    word = (word & ~mask) | ((val32 << slot.bit_offset) & mask);

    const uint32_t le_word = to_little_endian32(word);
    std::memcpy(buffer, &le_word, sizeof(uint32_t));
    return true;
}

/**
 * @brief Injects register operand into x86_64 ModR/M byte and extends REX prefix.
 */
bool inject_x86_modrm(uint8_t* buffer, uint8_t byte_length, const NeoOperandSlot& slot, int64_t value) noexcept {
    const size_t byte_pos = slot.bit_offset / 8;
    if (byte_pos >= byte_length) return false;

    const uint8_t bit_in_byte = slot.bit_offset % 8;
    const uint8_t reg_bits = static_cast<uint8_t>(value & 0x07);
    const uint8_t mask = static_cast<uint8_t>(((1u << slot.bit_width) - 1u) << bit_in_byte);
    buffer[byte_pos] = (buffer[byte_pos] & ~mask) | ((reg_bits << bit_in_byte) & mask);

    // If register ID is 8..15, extend via REX prefix (REX.B for bit 0, REX.R for bit 3)
    if (value >= 8 && byte_length > 0 && (buffer[0] & 0xF0) == 0x40) {
        if (bit_in_byte == 0) buffer[0] |= 0x01;
        else if (bit_in_byte == 3) buffer[0] |= 0x04;
    }
    return true;
}

/**
 * @brief Injects immediate or PIC displacement into variable-length x86_64 byte stream.
 */
bool inject_x86_displacement(uint8_t* buffer, uint8_t byte_length, const NeoOperandSlot& slot, int64_t value) noexcept {
    const size_t byte_pos = slot.bit_offset / 8;

    if (slot.bit_width == 32 || slot.slot_type == OPERAND_PIC_REL_TAG) {
        if (byte_pos + 4 > byte_length) return false;
        const uint32_t val32 = to_little_endian32(static_cast<uint32_t>(value));
        std::memcpy(buffer + byte_pos, &val32, sizeof(uint32_t));
        return true;
    }
    if (slot.bit_width == 8) {
        if (byte_pos + 1 > byte_length) return false;
        buffer[byte_pos] = static_cast<uint8_t>(value);
        return true;
    }
    if (slot.bit_width == 16) {
        if (byte_pos + 2 > byte_length) return false;
        const uint16_t val16 = to_little_endian16(static_cast<uint16_t>(value));
        std::memcpy(buffer + byte_pos, &val16, sizeof(uint16_t));
        return true;
    }
    if (slot.bit_width == 64) {
        if (byte_pos + 8 > byte_length) return false;
        const uint64_t val64 = to_little_endian64(static_cast<uint64_t>(value));
        std::memcpy(buffer + byte_pos, &val64, sizeof(uint64_t));
        return true;
    }
    return false;
}

} // namespace

bool NeoEmitter::is_value_in_bounds(const NeoOperandSlot& slot, int64_t value, uint16_t arch_id) noexcept {
    if (slot.slot_type == OPERAND_NONE) return true;
    if (slot.bit_width == 0 || slot.bit_width > 64) return false;
    if (slot.slot_type == OPERAND_REG_GPR) return check_register_bounds(value, slot.bit_width, arch_id);
    if (slot.slot_type == OPERAND_PIC_REL_TAG) return check_signed_bounds(value, slot.bit_width);
    return check_immediate_bounds(value, slot.bit_width);
}

bool NeoEmitter::inject_operand(uint8_t* buffer, uint8_t byte_length, const NeoOperandSlot& slot, int64_t value, uint16_t arch_id) noexcept {
    if (!buffer || byte_length == 0) return false;
    if (slot.slot_type == OPERAND_NONE) return true;
    if (!is_value_in_bounds(slot, value, arch_id)) return false;

    if (arch_id == USX_ARCH_AARCH64 || arch_id == USX_ARCH_RISCV64) {
        return inject_aarch64_riscv(buffer, byte_length, slot, value);
    }
    if (slot.slot_type == OPERAND_REG_GPR) {
        return inject_x86_modrm(buffer, byte_length, slot, value);
    }
    return inject_x86_displacement(buffer, byte_length, slot, value);
}

bool NeoEmitter::emit_instruction(
    const NeoInstructionTemplate& tmpl,
    std::span<const int64_t> operand_values,
    uint16_t arch_id,
    std::vector<uint8_t>& out_buffer)
{
    if (tmpl.byte_length == 0 || tmpl.byte_length > 16 || tmpl.operand_count > 4) return false;
    if (operand_values.size() < tmpl.operand_count) return false;

    uint8_t insn_buf[16]{};
    std::memcpy(insn_buf, tmpl.opcode_bytes, tmpl.byte_length);

    for (uint8_t i = 0; i < tmpl.operand_count; ++i) {
        if (!inject_operand(insn_buf, tmpl.byte_length, tmpl.slots[i], operand_values[i], arch_id)) {
            return false;
        }
    }
    out_buffer.insert(out_buffer.end(), insn_buf, insn_buf + tmpl.byte_length);
    return true;
}

std::vector<uint8_t> NeoEmitter::emit_instruction(
    const NeoInstructionTemplate& tmpl,
    std::span<const int64_t> operand_values,
    uint16_t arch_id)
{
    std::vector<uint8_t> result;
    if (!emit_instruction(tmpl, operand_values, arch_id, result)) return {};
    return result;
}

bool NeoEmitter::emit_instruction(
    const NeoInstructionTemplate& tmpl,
    std::initializer_list<int64_t> operand_values,
    uint16_t arch_id,
    std::vector<uint8_t>& out_buffer)
{
    return emit_instruction(tmpl, std::span<const int64_t>(operand_values.begin(), operand_values.size()), arch_id, out_buffer);
}

std::vector<uint8_t> NeoEmitter::emit_instruction(
    const NeoInstructionTemplate& tmpl,
    std::initializer_list<int64_t> operand_values,
    uint16_t arch_id)
{
    return emit_instruction(tmpl, std::span<const int64_t>(operand_values.begin(), operand_values.size()), arch_id);
}

} // namespace neo
