/**
 * @file test_expansion.cpp
 * @brief Unit tests for register virtualization and x86_64 stack spill sequences.
 */

#include <neo/common.hpp>
#include <neo/parser.hpp>
#include <neo/register_alloc.hpp>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

void test_aarch64_direct_mapping() {
    const auto insns = neo::NeoRegisterAllocator::expand_source(
        "ADD r20, r21, r22", neo::USX_ARCH_AARCH64);
    assert(insns.size() == 1);
    assert(insns[0].mnemonic == "ADD");
    assert(!insns[0].is_spill_sequence);
    assert(insns[0].operands.size() == 3);
    assert(insns[0].operands[0].reg == 20 && insns[0].operands[0].reg_name == "X20");
    assert(insns[0].operands[1].reg == 21 && insns[0].operands[1].reg_name == "X21");
    assert(insns[0].operands[2].reg == 22 && insns[0].operands[2].reg_name == "X22");
    assert(insns[0].to_string() == "ADD X20, X21, X22");

    const auto m_sp = neo::NeoRegisterAllocator::map_register(neo::REG_SP, true, neo::USX_ARCH_AARCH64);
    assert(m_sp.hardware_id == 31 && m_sp.name == "SP" && !m_sp.is_spilled);
    const auto m_fp = neo::NeoRegisterAllocator::map_register(neo::REG_FP, true, neo::USX_ARCH_AARCH64);
    assert(m_fp.hardware_id == 29 && m_fp.name == "X29" && !m_fp.is_spilled);
    const auto m_zr = neo::NeoRegisterAllocator::map_register(neo::REG_ZR, true, neo::USX_ARCH_AARCH64);
    assert(m_zr.hardware_id == 31 && m_zr.name == "XZR" && !m_zr.is_spilled);
    const auto m_r31 = neo::NeoRegisterAllocator::map_register(31, false, neo::USX_ARCH_AARCH64);
    assert(m_r31.hardware_id == 19 && m_r31.name == "X19" && !m_r31.is_spilled);
}

void test_riscv64_direct_mapping() {
    const auto insns = neo::NeoRegisterAllocator::expand_source(
        "ADD r20, r21, r22", neo::USX_ARCH_RISCV64);
    assert(insns.size() == 1);
    assert(insns[0].mnemonic == "ADD");
    assert(!insns[0].is_spill_sequence);
    assert(insns[0].operands.size() == 3);
    assert(insns[0].operands[0].reg == 20 && insns[0].operands[0].reg_name == "x20");
    assert(insns[0].operands[1].reg == 21 && insns[0].operands[1].reg_name == "x21");
    assert(insns[0].operands[2].reg == 22 && insns[0].operands[2].reg_name == "x22");
    assert(insns[0].to_string() == "ADD x20, x21, x22");

    const auto m_sp = neo::NeoRegisterAllocator::map_register(neo::REG_SP, true, neo::USX_ARCH_RISCV64);
    assert(m_sp.hardware_id == 2 && m_sp.name == "x2" && !m_sp.is_spilled);
    const auto m_fp = neo::NeoRegisterAllocator::map_register(neo::REG_FP, true, neo::USX_ARCH_RISCV64);
    assert(m_fp.hardware_id == 8 && m_fp.name == "x8" && !m_fp.is_spilled);
    const auto m_zr = neo::NeoRegisterAllocator::map_register(neo::REG_ZR, true, neo::USX_ARCH_RISCV64);
    assert(m_zr.hardware_id == 0 && m_zr.name == "x0" && !m_zr.is_spilled);
}

void test_x86_non_spill_boundary_r13() {
    const auto insns = neo::NeoRegisterAllocator::expand_source(
        "ADD r13, r0, r1", neo::USX_ARCH_X86_64);
    assert(insns.size() == 1);
    assert(insns[0].mnemonic == "ADD");
    assert(!insns[0].is_spill_sequence);
    assert(insns[0].operands.size() == 3);
    assert(insns[0].operands[0].reg == 15 && insns[0].operands[0].reg_name == "R15");
    assert(insns[0].operands[1].reg == 0 && insns[0].operands[1].reg_name == "RAX");
    assert(insns[0].operands[2].reg == 1 && insns[0].operands[2].reg_name == "RCX");
    assert(insns[0].to_string() == "ADD R15, RAX, RCX");

    const auto m_r0 = neo::NeoRegisterAllocator::map_register(0, false, neo::USX_ARCH_X86_64);
    assert(m_r0.hardware_id == 0 && m_r0.name == "RAX" && !m_r0.is_spilled);
    const auto m_r13 = neo::NeoRegisterAllocator::map_register(13, false, neo::USX_ARCH_X86_64);
    assert(m_r13.hardware_id == 15 && m_r13.name == "R15" && !m_r13.is_spilled);
    assert(!neo::NeoRegisterAllocator::is_spilled(13, neo::USX_ARCH_X86_64));
}

void test_x86_spill_boundary_r14() {
    assert(neo::NeoRegisterAllocator::is_spilled(14, neo::USX_ARCH_X86_64));
    assert(neo::NeoRegisterAllocator::spill_offset(14) == 8);

    const auto insns = neo::NeoRegisterAllocator::expand_source(
        "ADD r14, r0, r1", neo::USX_ARCH_X86_64);
    assert(insns.size() == 3);
    assert(insns[0].is_spill_sequence && insns[1].is_spill_sequence && insns[2].is_spill_sequence);

    // Scratch load, compute with RCX, scratch store to [rbp - 8]
    assert(insns[0].mnemonic == "MOV" && insns[0].to_string() == "MOV RAX, [rbp - 8]");
    assert(insns[1].mnemonic == "ADD" && insns[1].to_string() == "ADD RAX, RCX");
    assert(insns[2].mnemonic == "MOV" && insns[2].to_string() == "MOV [rbp - 8], RAX");
}

void test_x86_dual_spill_max_register_r31() {
    assert(neo::NeoRegisterAllocator::is_spilled(31, neo::USX_ARCH_X86_64));
    assert(neo::NeoRegisterAllocator::spill_offset(31) == 144);
    assert(neo::NeoRegisterAllocator::spill_offset(30) == 136);
    assert(neo::NeoRegisterAllocator::spill_offset(29) == 128);

    const auto insns = neo::NeoRegisterAllocator::expand_source(
        "ADD r31, r30, r29", neo::USX_ARCH_X86_64);
    assert(insns.size() == 4);
    for (const auto& in : insns) {
        assert(in.is_spill_sequence);
    }

    // Load src1 to RAX, src2 to secondary scratch RDX, compute, store to [rbp - 144]
    assert(insns[0].mnemonic == "MOV" && insns[0].to_string() == "MOV RAX, [rbp - 136]");
    assert(insns[1].mnemonic == "MOV" && insns[1].to_string() == "MOV RDX, [rbp - 128]");
    assert(insns[2].mnemonic == "ADD" && insns[2].to_string() == "ADD RAX, RDX");
    assert(insns[3].mnemonic == "MOV" && insns[3].to_string() == "MOV [rbp - 144], RAX");
}

void test_spill_offset_and_arch_checks() {
    for (uint8_t r = 14; r <= 31; ++r) {
        const int32_t expected = static_cast<int32_t>((r - 14 + 1) * 8);
        assert(neo::NeoRegisterAllocator::spill_offset(r) == expected);
        assert(neo::NeoRegisterAllocator::is_spilled(r, neo::USX_ARCH_X86_64));
        assert(!neo::NeoRegisterAllocator::is_spilled(r, neo::USX_ARCH_AARCH64));
        assert(!neo::NeoRegisterAllocator::is_spilled(r, neo::USX_ARCH_RISCV64));
    }
    assert(neo::NeoRegisterAllocator::spill_offset(13) == 0);
    assert(neo::NeoRegisterAllocator::spill_offset(0) == 0);

    assert(neo::NeoRegisterAllocator::resolve_arch_id("x86_64") == neo::USX_ARCH_X86_64);
    assert(neo::NeoRegisterAllocator::resolve_arch_id("aarch64") == neo::USX_ARCH_AARCH64);
    assert(neo::NeoRegisterAllocator::resolve_arch_id("riscv64") == neo::USX_ARCH_RISCV64);
}

void test_x86_assignment_and_stack_spills() {
    // ASN r14, r0 -> MOV [rbp - 8], RAX
    const auto asn_store = neo::NeoRegisterAllocator::expand_source(
        "ASN r14, r0", neo::USX_ARCH_X86_64);
    assert(asn_store.size() == 1 && asn_store[0].to_string() == "MOV [rbp - 8], RAX");

    // ASN r0, r14 -> MOV RAX, [rbp - 8]
    const auto asn_load = neo::NeoRegisterAllocator::expand_source(
        "ASN r0, r14", neo::USX_ARCH_X86_64);
    assert(asn_load.size() == 1 && asn_load[0].to_string() == "MOV RAX, [rbp - 8]");

    // ASN r14, r15 -> MOV RAX, [rbp - 16] followed by MOV [rbp - 8], RAX
    const auto asn_both = neo::NeoRegisterAllocator::expand_source(
        "ASN r14, r15", neo::USX_ARCH_X86_64);
    assert(asn_both.size() == 2);
    assert(asn_both[0].to_string() == "MOV RAX, [rbp - 16]");
    assert(asn_both[1].to_string() == "MOV [rbp - 8], RAX");

    // PUSH r14 -> MOV RAX, [rbp - 8] followed by PUSH RAX
    const auto push_seq = neo::NeoRegisterAllocator::expand_source(
        "PUSH r14", neo::USX_ARCH_X86_64);
    assert(push_seq.size() == 2);
    assert(push_seq[0].to_string() == "MOV RAX, [rbp - 8]");
    assert(push_seq[1].to_string() == "PUSH RAX");

    // POP r14 -> POP RAX followed by MOV [rbp - 8], RAX
    const auto pop_seq = neo::NeoRegisterAllocator::expand_source(
        "POP r14", neo::USX_ARCH_X86_64);
    assert(pop_seq.size() == 2);
    assert(pop_seq[0].to_string() == "POP RAX");
    assert(pop_seq[1].to_string() == "MOV [rbp - 8], RAX");
}

} // namespace

int main() {
    std::cout << "[TEST] Running test_expansion..." << std::endl;
    test_aarch64_direct_mapping();
    test_riscv64_direct_mapping();
    test_x86_non_spill_boundary_r13();
    test_x86_spill_boundary_r14();
    test_x86_dual_spill_max_register_r31();
    test_spill_offset_and_arch_checks();
    test_x86_assignment_and_stack_spills();
    std::cout << "[TEST] All test_expansion suites passed successfully!" << std::endl;
    return 0;
}
