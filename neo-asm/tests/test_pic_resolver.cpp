/**
 * @file test_pic_resolver.cpp
 * @brief Unit tests for Position-Independent Code (PIC) branch displacement resolver.
 */

#include <neo/common.hpp>
#include <neo/parser.hpp>
#include <neo/pic_resolver.hpp>

#include <cassert>
#include <iostream>
#include <string>

namespace {

void test_forward_branches() {
    // x86_64: disp = 0x200 - (0x100 + 5) = 251 (+251)
    const auto r_x86 = neo::NeoPicResolver::calculate_displacement(
        0x200, 0x100, 5, 32, neo::USX_ARCH_X86_64);
    assert(r_x86.success && r_x86.displacement == 251);

    // AArch64: disp = (0x200 - 0x100) >> 2 = 64 (+64 words)
    const auto r_arm = neo::NeoPicResolver::calculate_displacement(
        0x200, 0x100, 4, 26, neo::USX_ARCH_AARCH64);
    assert(r_arm.success && r_arm.displacement == 64);

    // RISC-V 64: disp = 0x200 - 0x100 = 256 (+256 bytes)
    const auto r_rv = neo::NeoPicResolver::calculate_displacement(
        0x200, 0x100, 4, 20, neo::USX_ARCH_RISCV64);
    assert(r_rv.success && r_rv.displacement == 256);
}

void test_backward_branches() {
    // x86_64: disp = 0x100 - (0x200 + 5) = -261
    const auto r_x86 = neo::NeoPicResolver::calculate_displacement(
        0x100, 0x200, 5, 32, neo::USX_ARCH_X86_64);
    assert(r_x86.success && r_x86.displacement == -261);

    // AArch64: disp = (0x100 - 0x200) >> 2 = -64
    const auto r_arm = neo::NeoPicResolver::calculate_displacement(
        0x100, 0x200, 4, 26, neo::USX_ARCH_AARCH64);
    assert(r_arm.success && r_arm.displacement == -64);

    // RISC-V 64: disp = 0x100 - 0x200 = -256
    const auto r_rv = neo::NeoPicResolver::calculate_displacement(
        0x100, 0x200, 4, 20, neo::USX_ARCH_RISCV64);
    assert(r_rv.success && r_rv.displacement == -256);
}

void test_zero_displacement() {
    // x86_64: target == current + length -> disp == 0
    const auto r_x86 = neo::NeoPicResolver::calculate_displacement(
        0x105, 0x100, 5, 32, neo::USX_ARCH_X86_64);
    assert(r_x86.success && r_x86.displacement == 0);

    // AArch64: target == current -> disp == 0 (branch to self)
    const auto r_arm = neo::NeoPicResolver::calculate_displacement(
        0x100, 0x100, 4, 26, neo::USX_ARCH_AARCH64);
    assert(r_arm.success && r_arm.displacement == 0);

    // RISC-V 64: target == current -> disp == 0 (branch to self)
    const auto r_rv = neo::NeoPicResolver::calculate_displacement(
        0x100, 0x100, 4, 20, neo::USX_ARCH_RISCV64);
    assert(r_rv.success && r_rv.displacement == 0);
}

void test_alignment_restrictions() {
    // AArch64 requires 4-byte instruction alignment
    const auto r_arm_bad = neo::NeoPicResolver::calculate_displacement(
        0x102, 0x100, 4, 26, neo::USX_ARCH_AARCH64);
    assert(!r_arm_bad.success && r_arm_bad.error_code == neo::PicErrorCode::InvalidAlignment);

    // RISC-V requires 2-byte instruction alignment
    const auto r_rv_bad = neo::NeoPicResolver::calculate_displacement(
        0x101, 0x100, 4, 20, neo::USX_ARCH_RISCV64);
    assert(!r_rv_bad.success && r_rv_bad.error_code == neo::PicErrorCode::InvalidAlignment);
}

void test_slot_bitwidth_overflow() {
    // 8-bit slot on x86: signed range [-128, 127]
    const auto r_ok_pos = neo::NeoPicResolver::calculate_displacement(
        0x17F, 0x100, 0, 8, neo::USX_ARCH_X86_64);
    assert(r_ok_pos.success && r_ok_pos.displacement == 127);

    const auto r_err_pos = neo::NeoPicResolver::calculate_displacement(
        0x180, 0x100, 0, 8, neo::USX_ARCH_X86_64);
    assert(!r_err_pos.success && r_err_pos.error_code == neo::PicErrorCode::DisplacementOutOfRange);

    const auto r_ok_neg = neo::NeoPicResolver::calculate_displacement(
        0x080, 0x100, 0, 8, neo::USX_ARCH_X86_64);
    assert(r_ok_neg.success && r_ok_neg.displacement == -128);

    const auto r_err_neg = neo::NeoPicResolver::calculate_displacement(
        0x07F, 0x100, 0, 8, neo::USX_ARCH_X86_64);
    assert(!r_err_neg.success && r_err_neg.error_code == neo::PicErrorCode::DisplacementOutOfRange);
}

void test_hierarchical_dotted_labels() {
    neo::NeoPicResolver resolver;
    resolver.record_label("label_a", 0x1000);
    resolver.record_label("label_a.label_b", 0x1020);
    resolver.record_label("label_a.label_b.label_c", 0x1040);
    resolver.record_label("label_a.label_d", 0x1060);

    // Exact fully qualified path lookup
    const auto r1 = resolver.resolve_displacement(
        "label_a.label_b.label_c", 0x1000, 5, 32, neo::USX_ARCH_X86_64);
    assert(r1.success && r1.displacement == 0x1040 - 0x1005);

    // Local label_c inside scope label_a.label_b.label_c
    const auto r2 = resolver.resolve_displacement(
        "label_c", 0x1040, 5, 32, neo::USX_ARCH_X86_64, "label_a.label_b.label_c");
    assert(r2.success && r2.displacement == 0x1040 - 0x1045);

    // Sibling label_b from inside scope label_a.label_b.label_c
    const auto r3 = resolver.resolve_displacement(
        "label_b", 0x1040, 5, 32, neo::USX_ARCH_X86_64, "label_a.label_b.label_c");
    assert(r3.success && r3.displacement == 0x1020 - 0x1045);

    // Sibling relative path label_b.label_c from inside scope label_a.label_d
    const auto r4 = resolver.resolve_displacement(
        "label_b.label_c", 0x1060, 5, 32, neo::USX_ARCH_X86_64, "label_a.label_d");
    assert(r4.success && r4.displacement == 0x1040 - 0x1065);
}

void test_undefined_label_tag() {
    neo::NeoPicResolver resolver;
    resolver.record_label("known_label", 0x1000);

    const auto res = resolver.resolve_displacement(
        "unknown_label", 0x1000, 5, 32, neo::USX_ARCH_X86_64);
    assert(!res.success);
    assert(res.error_code == neo::PicErrorCode::UndefinedLabel);
    assert(res.error_message.find("unknown_label") != std::string::npos);
}

void test_two_pass_scanning() {
    neo::NeoPicResolver resolver;
    resolver.scan_instruction_lengths({5, 4, 4}, 0x1000);

    assert(resolver.get_instruction_pc(0) == 0x1000);
    assert(resolver.get_instruction_pc(1) == 0x1005);
    assert(resolver.get_instruction_pc(2) == 0x1009);

    resolver.record_label("target_loop", 0x1000);
    const auto res = resolver.resolve_displacement_for_instruction(
        2, "target_loop", 4, 32, neo::USX_ARCH_X86_64);
    assert(res.success && res.displacement == -13);
}

} // namespace

int main() {
    test_forward_branches();
    test_backward_branches();
    test_zero_displacement();
    test_alignment_restrictions();
    test_slot_bitwidth_overflow();
    test_hierarchical_dotted_labels();
    test_undefined_label_tag();
    test_two_pass_scanning();

    std::cout << "[PASS] PIC branch displacement resolver test suite\n";
    return 0;
}
