/**
 * @file test_assembler_spill_and_pic.cpp
 * @brief Integration tests for register spill sequences and PIC displacement resolution.
 */

#include "test_assembler_suites.hpp"
#include "neo/compiler.hpp"
#include "neo/translator.hpp"

#include <cassert>
#include <iostream>
#include <vector>

namespace neo::test {

namespace {

NeoDatabase load_x86_database(const std::filesystem::path& tmpl_dir) {
    const auto db_path = std::filesystem::temp_directory_path() / "test_asm_x86.neodb";
    if (!std::filesystem::exists(db_path)) {
        assert(NeoTranslator::yaml_to_neodb(tmpl_dir / "neo_asm_x86_64.yaml", db_path));
    }
    NeoDatabase db;
    assert(db.load_file(db_path));
    return db;
}

void test_x86_spill_boundary_r14(const std::filesystem::path& tmpl_dir) {
    const auto db = load_x86_database(tmpl_dir);

    // ADD r14, r0, r1:
    // r14 spills to [rbp - 8]. Emits scratch load, compute with RCX, store back.
    // 1. MOV RAX, [rbp - 8] -> 48 8B 85 F8 FF FF FF (7 bytes)
    // 2. ADD RAX, RCX       -> 48 01 C8 (3 bytes)
    // 3. MOV [rbp - 8], RAX -> 48 89 85 F8 FF FF FF (7 bytes)
    std::vector<uint8_t> bin;
    assert(NeoCompiler::compile_source("ADD r14, r0, r1\n", db, bin));

    assert(bin.size() == 17);
    assert(bin[0] == 0x48 && bin[1] == 0x8B); // MOV RAX, [rbp - 8]
    assert(bin[7] == 0x48 && bin[8] == 0x01 && bin[9] == 0xC8); // ADD RAX, RCX
    assert(bin[10] == 0x48 && bin[11] == 0x89); // MOV [rbp - 8], RAX
}

void test_x86_non_spill_r13(const std::filesystem::path& tmpl_dir) {
    const auto db = load_x86_database(tmpl_dir);

    // ADD r13, r0, r1 on x86_64:
    // r13 maps to R15 (native register 15), r0 maps to RAX (0), r1 maps to RCX (1).
    // Template 0: MOV R15, RAX -> 49 89 C7 (3 bytes)
    // Template 1: ADD R15, RCX -> 49 01 CF (3 bytes)
    // Emits 6 bytes with no stack spills.
    std::vector<uint8_t> bin;
    assert(NeoCompiler::compile_source("ADD r13, r0, r1\n", db, bin));

    assert(bin.size() == 6);
    assert(bin[0] == 0x49 && bin[1] == 0x89 && bin[2] == 0xC7);
    assert(bin[3] == 0x49 && bin[4] == 0x01 && bin[5] == 0xCF);
}

void test_pic_branch_displacements(const std::filesystem::path& tmpl_dir) {
    const auto db = load_x86_database(tmpl_dir);

    // Loop with backward relative branch:
    // loop:
    //   origin(SYSCALL)   ; 2 bytes (0x0F, 0x05) at PC 0
    //   JMP loop          ; 5 bytes (0xE9 disp32) at PC 2..6
    // target = 0, current = 2, insn_len = 5 -> disp = 0 - (2 + 5) = -7
    const std::string src =
        "loop:\n"
        "    origin(SYSCALL)\n"
        "    JMP loop\n";

    std::vector<uint8_t> bin;
    assert(NeoCompiler::compile_source(src, db, bin));

    assert(bin.size() == 7);
    assert(bin[0] == 0x0F && bin[1] == 0x05);
    assert(bin[2] == 0xE9);

    // Negative displacement -7: 0xFFFFFFF9 (little-endian: F9 FF FF FF)
    assert(bin[3] == 0xF9 && bin[4] == 0xFF && bin[5] == 0xFF && bin[6] == 0xFF);
}

} // namespace

void test_suite_spill_and_pic(const std::filesystem::path& tmpl_dir) {
    test_x86_spill_boundary_r14(tmpl_dir);
    test_x86_non_spill_r13(tmpl_dir);
    test_pic_branch_displacements(tmpl_dir);
    std::cout << "[PASS] Register spilling and PIC branch test suite\n";
}

} // namespace neo::test
