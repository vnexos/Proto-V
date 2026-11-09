/**
 * @file test_assembler_origin.cpp
 * @brief Integration tests verifying native passthrough instruction compilation.
 */

#include "test_assembler_suites.hpp"
#include "neo/compiler.hpp"
#include "neo/translator.hpp"

#include <cassert>
#include <iostream>
#include <vector>

namespace neo::test {

namespace {

void test_x86_syscall(const std::filesystem::path& tmpl_dir) {
    const auto yaml_path = tmpl_dir / "neo_asm_x86_64.yaml";
    const auto db_path = std::filesystem::temp_directory_path() / "test_orig_x86.neodb";
    assert(NeoTranslator::yaml_to_neodb(yaml_path, db_path));

    NeoDatabase db;
    assert(db.load_file(db_path));

    std::vector<uint8_t> bin;
    assert(NeoCompiler::compile_source("origin(SYSCALL)\n", db, bin));

    const std::vector<uint8_t> expected = {0x0F, 0x05};
    assert(bin == expected);
}

void test_aarch64_svc(const std::filesystem::path& tmpl_dir) {
    const auto yaml_path = tmpl_dir / "neo_asm_aarch64.yaml";
    const auto db_path = std::filesystem::temp_directory_path() / "test_orig_aarch64.neodb";
    assert(NeoTranslator::yaml_to_neodb(yaml_path, db_path));

    NeoDatabase db;
    assert(db.load_file(db_path));

    std::vector<uint8_t> bin;
    assert(NeoCompiler::compile_source("origin(SVC 0)\n", db, bin));

    const std::vector<uint8_t> expected = {0xD4, 0x00, 0x00, 0x01};
    assert(bin == expected);
}

void test_riscv64_ecall(const std::filesystem::path& tmpl_dir) {
    const auto yaml_path = tmpl_dir / "neo_asm_riscv64.yaml";
    const auto db_path = std::filesystem::temp_directory_path() / "test_orig_riscv.neodb";
    assert(NeoTranslator::yaml_to_neodb(yaml_path, db_path));

    NeoDatabase db;
    assert(db.load_file(db_path));

    std::vector<uint8_t> bin;
    assert(NeoCompiler::compile_source("origin(ECALL)\n", db, bin));

    const std::vector<uint8_t> expected = {0x00, 0x00, 0x00, 0x73};
    assert(bin == expected);
}

} // namespace

void test_suite_origin(const std::filesystem::path& tmpl_dir) {
    test_x86_syscall(tmpl_dir);
    test_aarch64_svc(tmpl_dir);
    test_riscv64_ecall(tmpl_dir);
    std::cout << "[PASS] Native origin passthrough test suite\n";
}

} // namespace neo::test
