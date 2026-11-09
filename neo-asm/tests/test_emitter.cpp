/**
 * @file test_emitter.cpp
 * @brief Unit test suite verifying architecture-aware instruction bitfield injection.
 */

#include "neo/emitter.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

// Test AArch64 32-bit instruction word masking and register ID injection
void test_aarch64_bitfield_injection() {
    // AArch64 ADD Xd, Xn, Xm base word: 0x8B000000 (little-endian byte representation)
    uint32_t base_word = 0x8B000000u;
    uint8_t buffer[4]{};
    std::memcpy(buffer, &base_word, sizeof(uint32_t));

    // Slot 0: Rd in bits 0..4 (mask 0x0000001F, bit_width 5)
    neo::NeoOperandSlot slot_rd{neo::OPERAND_REG_GPR, 0, 5, 0, 0x0000001Fu};
    // Slot 1: Rn in bits 5..9 (mask 0x000003E0, bit_width 5)
    neo::NeoOperandSlot slot_rn{neo::OPERAND_REG_GPR, 5, 5, 0, 0x000003E0u};
    // Slot 2: Rm in bits 16..20 (mask 0x001F0000, bit_width 5)
    neo::NeoOperandSlot slot_rm{neo::OPERAND_REG_GPR, 16, 5, 0, 0x001F0000u};

    // Inject ADD X20, X21, X22: Rd=20 (0x14), Rn=21 (0x15), Rm=22 (0x16)
    assert(neo::NeoEmitter::inject_operand(buffer, 4, slot_rd, 20, neo::USX_ARCH_AARCH64));
    assert(neo::NeoEmitter::inject_operand(buffer, 4, slot_rn, 21, neo::USX_ARCH_AARCH64));
    assert(neo::NeoEmitter::inject_operand(buffer, 4, slot_rm, 22, neo::USX_ARCH_AARCH64));

    // Expected 32-bit little-endian instruction word: 0x8B1602B4
    uint32_t result_word = 0;
    std::memcpy(&result_word, buffer, sizeof(uint32_t));
    assert(result_word == 0x8B1602B4u);

    // Verify machine code byte order: 0xB4, 0x02, 0x16, 0x8B
    assert(buffer[0] == 0xB4 && buffer[1] == 0x02 && buffer[2] == 0x16 && buffer[3] == 0x8B);
}

// Test RISC-V 64 32-bit instruction word masking and register ID injection
void test_riscv64_bitfield_injection() {
    // RISC-V ADD rd, rs1, rs2 base word: 0x00000033 (little-endian byte representation)
    uint32_t base_word = 0x00000033u;
    uint8_t buffer[4]{};
    std::memcpy(buffer, &base_word, sizeof(uint32_t));

    // Slot 0: rd in bits 7..11 (mask 0x00000F80, bit_width 5)
    neo::NeoOperandSlot slot_rd{neo::OPERAND_REG_GPR, 7, 5, 0, 0x00000F80u};
    // Slot 1: rs1 in bits 15..19 (mask 0x000F8000, bit_width 5)
    neo::NeoOperandSlot slot_rs1{neo::OPERAND_REG_GPR, 15, 5, 0, 0x000F8000u};
    // Slot 2: rs2 in bits 20..24 (mask 0x01F00000, bit_width 5)
    neo::NeoOperandSlot slot_rs2{neo::OPERAND_REG_GPR, 20, 5, 0, 0x01F00000u};

    // Inject ADD x20, x21, x22: rd=20, rs1=21, rs2=22
    assert(neo::NeoEmitter::inject_operand(buffer, 4, slot_rd, 20, neo::USX_ARCH_RISCV64));
    assert(neo::NeoEmitter::inject_operand(buffer, 4, slot_rs1, 21, neo::USX_ARCH_RISCV64));
    assert(neo::NeoEmitter::inject_operand(buffer, 4, slot_rs2, 22, neo::USX_ARCH_RISCV64));

    // Expected 32-bit word: 0x016A8A33 (bytes: 0x33, 0x8A, 0x6A, 0x01)
    uint32_t result_word = 0;
    std::memcpy(&result_word, buffer, sizeof(uint32_t));
    assert(result_word == 0x016A8A33u);
    assert(buffer[0] == 0x33 && buffer[1] == 0x8A && buffer[2] == 0x6A && buffer[3] == 0x01);
}

// Test x86_64 ModR/M register/rm byte-relative injection and REX prefix
void test_x86_64_modrm_injection() {
    // Template for MOV r/m64, r64: 48 89 C0 (REX.W + MOV + Mod=11 reg/rm direct)
    uint8_t buffer[3] = {0x48, 0x89, 0xC0};

    // Slot 0: r/m field at bit_offset 16 (shift 0 in byte 2), width 3
    neo::NeoOperandSlot slot_rm{neo::OPERAND_REG_GPR, 16, 3, 0, 0x070000u};
    // Slot 1: reg field at bit_offset 19 (shift 3 in byte 2), width 3
    neo::NeoOperandSlot slot_reg{neo::OPERAND_REG_GPR, 19, 3, 0, 0x380000u};

    // Case A: MOV RAX, RCX (RAX=0, RCX=1) -> 48 89 C8
    assert(neo::NeoEmitter::inject_operand(buffer, 3, slot_rm, 0, neo::USX_ARCH_X86_64));
    assert(neo::NeoEmitter::inject_operand(buffer, 3, slot_reg, 1, neo::USX_ARCH_X86_64));
    assert(buffer[0] == 0x48 && buffer[1] == 0x89 && buffer[2] == 0xC8);

    // Case B: High registers R8, R9 (R8=8, R9=9) -> Sets REX.B (bit 0) and REX.R (bit 2)
    uint8_t buf_rex[3] = {0x48, 0x89, 0xC0};
    assert(neo::NeoEmitter::inject_operand(buf_rex, 3, slot_rm, 8, neo::USX_ARCH_X86_64));
    assert(neo::NeoEmitter::inject_operand(buf_rex, 3, slot_reg, 9, neo::USX_ARCH_X86_64));
    assert(buf_rex[0] == 0x4D); // 0x48 | 0x01 (REX.B) | 0x04 (REX.R) = 0x4D
    assert(buf_rex[1] == 0x89 && buf_rex[2] == 0xC8);
}

// Test x86_64 32-bit relative PIC displacements in jump templates
void test_x86_64_pic_displacement() {
    // JMP rel32 template: E9 00 00 00 00 (5 bytes)
    uint8_t buffer[5] = {0xE9, 0x00, 0x00, 0x00, 0x00};
    neo::NeoOperandSlot slot_pic{neo::OPERAND_PIC_REL_TAG, 8, 32, 0, 0xFFFFFFFFu};

    // Positive displacement +0x123456
    assert(neo::NeoEmitter::inject_operand(buffer, 5, slot_pic, 0x123456, neo::USX_ARCH_X86_64));
    assert(buffer[0] == 0xE9 && buffer[1] == 0x56 && buffer[2] == 0x34 && buffer[3] == 0x12 && buffer[4] == 0x00);

    // Negative displacement -4 (two's complement 0xFFFFFFFC)
    assert(neo::NeoEmitter::inject_operand(buffer, 5, slot_pic, -4, neo::USX_ARCH_X86_64));
    assert(buffer[0] == 0xE9 && buffer[1] == 0xFC && buffer[2] == 0xFF && buffer[3] == 0xFF && buffer[4] == 0xFF);

    // Zero displacement 0
    assert(neo::NeoEmitter::inject_operand(buffer, 5, slot_pic, 0, neo::USX_ARCH_X86_64));
    assert(buffer[0] == 0xE9 && buffer[1] == 0x00 && buffer[2] == 0x00 && buffer[3] == 0x00 && buffer[4] == 0x00);
}

// Test operand bit-width bounds checking to prevent corruption
void test_bounds_and_overflow_prevention() {
    uint8_t buffer[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    neo::NeoOperandSlot slot_5bit{neo::OPERAND_REG_GPR, 0, 5, 0, 0x0000001Fu};

    // AArch64 / RISC-V 5-bit register bounds: 32 must be rejected
    assert(!neo::NeoEmitter::inject_operand(buffer, 4, slot_5bit, 32, neo::USX_ARCH_AARCH64));
    // Verify buffer was uncorrupted
    assert(buffer[0] == 0xAA && buffer[1] == 0xBB && buffer[2] == 0xCC && buffer[3] == 0xDD);

    // Negative register IDs must be rejected
    assert(!neo::NeoEmitter::inject_operand(buffer, 4, slot_5bit, -1, neo::USX_ARCH_AARCH64));

    // x86_64 GPR register bounds: 16 must be rejected for native GPRs
    neo::NeoOperandSlot slot_x86_reg{neo::OPERAND_REG_GPR, 16, 3, 0, 0x070000u};
    assert(!neo::NeoEmitter::inject_operand(buffer, 4, slot_x86_reg, 16, neo::USX_ARCH_X86_64));

    // PIC signed displacement exceeding 32-bit range must be rejected
    neo::NeoOperandSlot slot_pic{neo::OPERAND_PIC_REL_TAG, 8, 32, 0, 0xFFFFFFFFu};
    uint8_t jmp_buf[5] = {0xE9, 0, 0, 0, 0};
    assert(!neo::NeoEmitter::inject_operand(jmp_buf, 5, slot_pic, 0x100000000LL, neo::USX_ARCH_X86_64));
    assert(!neo::NeoEmitter::inject_operand(jmp_buf, 5, slot_pic, -0x100000001LL, neo::USX_ARCH_X86_64));
}

// Test emit_instruction end-to-end emission from templates
void test_emit_instruction() {
    neo::NeoInstructionTemplate tmpl{};
    tmpl.byte_length = 3;
    tmpl.operand_count = 2;
    tmpl.opcode_bytes[0] = 0x48;
    tmpl.opcode_bytes[1] = 0x89;
    tmpl.opcode_bytes[2] = 0xC0;
    tmpl.slots[0] = {neo::OPERAND_REG_GPR, 16, 3, 0, 0x070000u};
    tmpl.slots[1] = {neo::OPERAND_REG_GPR, 19, 3, 0, 0x380000u};

    // Emit MOV RAX, RCX
    const auto bytes = neo::NeoEmitter::emit_instruction(tmpl, {0, 1}, neo::USX_ARCH_X86_64);
    assert(bytes.size() == 3);
    assert(bytes[0] == 0x48 && bytes[1] == 0x89 && bytes[2] == 0xC8);

    // Zero-operand instruction (e.g. SYSCALL: 0x0F, 0x05)
    neo::NeoInstructionTemplate sys_tmpl{};
    sys_tmpl.byte_length = 2;
    sys_tmpl.operand_count = 0;
    sys_tmpl.opcode_bytes[0] = 0x0F;
    sys_tmpl.opcode_bytes[1] = 0x05;

    const auto sys_bytes = neo::NeoEmitter::emit_instruction(sys_tmpl, {}, neo::USX_ARCH_X86_64);
    assert(sys_bytes.size() == 2);
    assert(sys_bytes[0] == 0x0F && sys_bytes[1] == 0x05);
}

} // namespace

int main() {
    std::cout << "[TEST] Running test_emitter suite..." << std::endl;

    test_aarch64_bitfield_injection();
    test_riscv64_bitfield_injection();
    test_x86_64_modrm_injection();
    test_x86_64_pic_displacement();
    test_bounds_and_overflow_prevention();
    test_emit_instruction();

    std::cout << "[PASS] test_emitter completed successfully." << std::endl;
    return 0;
}
