/**
 * @file test_assembler_arithmetic.cpp
 * @brief Integration tests verifying machine code bytes against reference assemblers.
 */

#include "test_assembler_suites.hpp"
#include "neo/compiler.hpp"
#include "neo/translator.hpp"

#include <cassert>
#include <iostream>
#include <vector>

namespace neo::test {

namespace {

void test_x86_64_arithmetic(const std::filesystem::path& tmpl_dir) {
    const auto yaml_path = tmpl_dir / "neo_asm_x86_64.yaml";
    const auto db_path = std::filesystem::temp_directory_path() / "test_asm_x86.neodb";
    assert(NeoTranslator::yaml_to_neodb(yaml_path, db_path));

    NeoDatabase db;
    assert(db.load_file(db_path));

    // 1. Native arithmetic sequence matching NASM:
    // mov rax, rcx (48 89 C8)
    // add rax, rdx (48 01 D0)
    // sub rax, rbx (48 29 D8)
    const std::string origin_src =
        "origin(MOV) r0, r1\n"
        "origin(ADD) r0, r2\n"
        "origin(SUB) r0, r3\n";

    std::vector<uint8_t> origin_bin;
    assert(NeoCompiler::compile_source(origin_src, db, origin_bin));

    const std::vector<uint8_t> expected_nasm = {
        0x48, 0x89, 0xC8,
        0x48, 0x01, 0xD0,
        0x48, 0x29, 0xD8
    };
    assert(origin_bin == expected_nasm);

    // 2. Neo-ASM standard 3-operand ADD r0, r1, r2:
    // Expands to MOV RAX, RCX + ADD RAX, RDX
    const std::string neo_src = "ADD r0, r1, r2\n";
    std::vector<uint8_t> neo_bin;
    assert(NeoCompiler::compile_source(neo_src, db, neo_bin));

    const std::vector<uint8_t> expected_neo_add = {
        0x48, 0x89, 0xC8,
        0x48, 0x01, 0xD0
    };
    assert(neo_bin == expected_neo_add);
}

void test_aarch64_arithmetic(const std::filesystem::path& tmpl_dir) {
    const auto yaml_path = tmpl_dir / "neo_asm_aarch64.yaml";
    const auto db_path = std::filesystem::temp_directory_path() / "test_asm_aarch64.neodb";
    assert(NeoTranslator::yaml_to_neodb(yaml_path, db_path));

    NeoDatabase db;
    assert(db.load_file(db_path));

    // AArch64 GNU as reference:
    // add x20, x21, x22 -> 0x8B1602B4 (bytes: B4 02 16 8B)
    // sub x20, x21, x22 -> 0xCB1602B4 (bytes: B4 02 16 CB)
    const std::string src =
        "ADD r20, r21, r22\n"
        "SUB r20, r21, r22\n";

    std::vector<uint8_t> bin;
    assert(NeoCompiler::compile_source(src, db, bin));

    const std::vector<uint8_t> expected_gnu_as = {
        0xB4, 0x02, 0x16, 0x8B,
        0xB4, 0x02, 0x16, 0xCB
    };
    assert(bin == expected_gnu_as);
}

void test_riscv64_arithmetic(const std::filesystem::path& tmpl_dir) {
    const auto yaml_path = tmpl_dir / "neo_asm_riscv64.yaml";
    const auto db_path = std::filesystem::temp_directory_path() / "test_asm_riscv64.neodb";
    assert(NeoTranslator::yaml_to_neodb(yaml_path, db_path));

    NeoDatabase db;
    assert(db.load_file(db_path));

    // RISC-V 64 GNU as reference:
    // add x20, x21, x22 -> 0x016A8A33 (bytes: 33 8A 6A 01)
    // sub x20, x21, x22 -> 0x416A8A33 (bytes: 33 8A 6A 41)
    const std::string src =
        "ADD r20, r21, r22\n"
        "SUB r20, r21, r22\n";

    std::vector<uint8_t> bin;
    assert(NeoCompiler::compile_source(src, db, bin));

    const std::vector<uint8_t> expected_riscv_as = {
        0x33, 0x8A, 0x6A, 0x01,
        0x33, 0x8A, 0x6A, 0x41
    };
    assert(bin == expected_riscv_as);
}

} // namespace

void test_suite_arithmetic(const std::filesystem::path& tmpl_dir) {
    test_x86_64_arithmetic(tmpl_dir);
    test_aarch64_arithmetic(tmpl_dir);
    test_riscv64_arithmetic(tmpl_dir);
    std::cout << "[PASS] Reference arithmetic matching suite\n";
}

} // namespace neo::test
