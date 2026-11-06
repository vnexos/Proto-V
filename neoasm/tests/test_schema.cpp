/**
 * @file test_schema.cpp
 * @brief Unit tests and static assertions verifying packed binary layouts.
 */

#include <neo/common.hpp>
#include <neo/schema.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

// Compile-time static assertions verifying exact packed structure sizes
static_assert(sizeof(neo::NeoDbHeader) == 64, "NeoDbHeader must be exactly 64 bytes");
static_assert(sizeof(neo::NeoIndexEntry) == 24, "NeoIndexEntry must be exactly 24 bytes");
static_assert(sizeof(neo::NeoInstructionTemplate) == 56, "NeoInstructionTemplate must be 56 bytes");
static_assert(sizeof(neo::NeoOperandSlot) == 8, "NeoOperandSlot must be exactly 8 bytes");

// Static assertions verifying field offsets for NeoDbHeader
static_assert(offsetof(neo::NeoDbHeader, magic) == 0, "magic offset must be 0");
static_assert(offsetof(neo::NeoDbHeader, format_version) == 8, "format_version offset must be 8");
static_assert(offsetof(neo::NeoDbHeader, arch_id) == 12, "arch_id offset must be 12");
static_assert(offsetof(neo::NeoDbHeader, arch_flags) == 14, "arch_flags offset must be 14");
static_assert(offsetof(neo::NeoDbHeader, strtab_offset) == 16, "strtab_offset offset must be 16");
static_assert(offsetof(neo::NeoDbHeader, strtab_size) == 24, "strtab_size offset must be 24");
static_assert(offsetof(neo::NeoDbHeader, strtab_count) == 28, "strtab_count offset must be 28");
static_assert(offsetof(neo::NeoDbHeader, index_offset) == 32, "index_offset offset must be 32");
static_assert(offsetof(neo::NeoDbHeader, index_bucket_count) == 40, "index_bucket_count offset must be 40");
static_assert(offsetof(neo::NeoDbHeader, index_entry_count) == 44, "index_entry_count offset must be 44");
static_assert(offsetof(neo::NeoDbHeader, template_offset) == 48, "template_offset offset must be 48");
static_assert(offsetof(neo::NeoDbHeader, template_size) == 56, "template_size offset must be 56");
static_assert(offsetof(neo::NeoDbHeader, header_crc32) == 60, "header_crc32 offset must be 60");

// Static assertions verifying field offsets for NeoIndexEntry
static_assert(offsetof(neo::NeoIndexEntry, name_str_offset) == 0, "name_str_offset offset must be 0");
static_assert(offsetof(neo::NeoIndexEntry, name_hash) == 4, "name_hash offset must be 4");
static_assert(offsetof(neo::NeoIndexEntry, flags) == 8, "flags offset must be 8");
static_assert(offsetof(neo::NeoIndexEntry, template_count) == 12, "template_count offset must be 12");
static_assert(offsetof(neo::NeoIndexEntry, reserved) == 14, "reserved offset must be 14");
static_assert(offsetof(neo::NeoIndexEntry, template_offset) == 16, "template_offset offset must be 16");

// Static assertions verifying field offsets for NeoOperandSlot
static_assert(offsetof(neo::NeoOperandSlot, slot_type) == 0, "slot_type offset must be 0");
static_assert(offsetof(neo::NeoOperandSlot, bit_offset) == 1, "bit_offset offset must be 1");
static_assert(offsetof(neo::NeoOperandSlot, bit_width) == 2, "bit_width offset must be 2");
static_assert(offsetof(neo::NeoOperandSlot, flags) == 3, "flags offset must be 3");
static_assert(offsetof(neo::NeoOperandSlot, mask) == 4, "mask offset must be 4");

// Static assertions verifying field offsets for NeoInstructionTemplate
static_assert(offsetof(neo::NeoInstructionTemplate, instruction_id) == 0, "instruction_id offset must be 0");
static_assert(offsetof(neo::NeoInstructionTemplate, byte_length) == 2, "byte_length offset must be 2");
static_assert(offsetof(neo::NeoInstructionTemplate, operand_count) == 3, "operand_count offset must be 3");
static_assert(offsetof(neo::NeoInstructionTemplate, flags) == 4, "flags offset must be 4");
static_assert(offsetof(neo::NeoInstructionTemplate, opcode_bytes) == 8, "opcode_bytes offset must be 8");
static_assert(offsetof(neo::NeoInstructionTemplate, slots) == 24, "slots offset must be 24");

/**
 * @brief Validates runtime sizing and offset layout of NeoDbHeader.
 */
static void test_header_layout() {
    neo::NeoDbHeader header{};
    std::memcpy(header.magic, "NEODB\0\0\0", 8);
    header.format_version = 0x00010000;
    header.arch_id = neo::USX_ARCH_X86_64;
    header.strtab_offset = 64;
    header.template_size = 560;

    assert(sizeof(header) == 64);
    assert(header.strtab_offset % 8 == 0);
    assert(header.template_count() == 10);
    std::cout << "[PASS] test_header_layout\n";
}

/**
 * @brief Validates runtime sizing and layout of NeoIndexEntry.
 */
static void test_index_entry_layout() {
    neo::NeoIndexEntry entry{};
    entry.name_str_offset = 16;
    entry.name_hash = 0x811c9dc5;
    entry.template_count = 2;
    entry.template_offset = 128;

    assert(sizeof(entry) == 24);
    assert(entry.template_offset % 8 == 0);
    std::cout << "[PASS] test_index_entry_layout\n";
}

/**
 * @brief Validates runtime sizing and layout of NeoOperandSlot and NeoInstructionTemplate.
 */
static void test_template_layout() {
    neo::NeoOperandSlot slot{};
    slot.slot_type = neo::OPERAND_REG_GPR;
    slot.bit_offset = 3;
    slot.bit_width = 3;
    slot.mask = 0x00000038;

    assert(sizeof(slot) == 8);

    neo::NeoInstructionTemplate tmpl{};
    tmpl.instruction_id = 42;
    tmpl.byte_length = 3;
    tmpl.operand_count = 1;
    tmpl.slots[0] = slot;

    assert(sizeof(tmpl) == 56);
    std::cout << "[PASS] test_template_layout\n";
}

/**
 * @brief Validates architecture constants and flag definitions.
 */
static void test_arch_constants() {
    assert(neo::USX_ARCH_X86_64 == 0x0001);
    assert(neo::USX_ARCH_AARCH64 == 0x0002);
    assert(neo::USX_ARCH_RISCV64 == 0x0004);
    assert(neo::USX_AFLAG_BIG_ENDIAN == 0x0001);

    std::cout << "[PASS] test_arch_constants\n";
}

/**
 * @brief Validates endianness helper conversion routines.
 */
static void test_endian_helpers() {
    const uint16_t v16 = 0x1234;
    const uint32_t v32 = 0x12345678;
    const uint64_t v64 = 0x123456789ABCDEF0ull;

    if constexpr (neo::is_little_endian()) {
        assert(neo::to_little_endian16(v16) == v16);
        assert(neo::to_little_endian32(v32) == v32);
        assert(neo::to_little_endian64(v64) == v64);
    }

    std::cout << "[PASS] test_endian_helpers\n";
}

int main() {
    test_header_layout();
    test_index_entry_layout();
    test_template_layout();
    test_arch_constants();
    test_endian_helpers();

    std::cout << "All schema layout and contract tests passed successfully.\n";
    return 0;
}
