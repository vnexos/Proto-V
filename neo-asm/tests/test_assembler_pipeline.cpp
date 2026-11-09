/**
 * @file test_assembler_pipeline.cpp
 * @brief Integration tests verifying flat binary files, absence of container headers, and preprocessor.
 */

#include "test_assembler_suites.hpp"
#include "neo/compiler.hpp"
#include "neo/translator.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

namespace neo::test {

namespace {

bool contains_container_headers(const std::vector<uint8_t>& bytes) {
    if (bytes.size() >= 4) {
        // Check ELF magic \x7fELF
        if (bytes[0] == 0x7F && bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F') return true;
        // Check Mach-O 32-bit and 64-bit magics
        const uint32_t magic = (static_cast<uint32_t>(bytes[0]) << 24) |
                               (static_cast<uint32_t>(bytes[1]) << 16) |
                               (static_cast<uint32_t>(bytes[2]) << 8)  |
                               static_cast<uint32_t>(bytes[3]);
        if (magic == 0xFEEDFACE || magic == 0xFEEDFACF || magic == 0xCEFAEDFE || magic == 0xCFFAEDFE) return true;
    }
    if (bytes.size() >= 2) {
        // Check PE / DOS executable magic 'MZ'
        if (bytes[0] == 'M' && bytes[1] == 'Z') return true;
    }
    return false;
}

std::vector<uint8_t> read_bin_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    assert(file.is_open());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void test_flat_binary_no_headers(const std::filesystem::path&) {
    const auto temp_dir = std::filesystem::temp_directory_path();
    const auto x86_neoa = temp_dir / "test_app_x86.neoa";
    const auto x86_bin = temp_dir / "test_app_x86.bin";
    const auto arm_neoa = temp_dir / "test_app_arm.neoa";
    const auto arm_bin = temp_dir / "test_app_arm.bin";

    // 1. Write sample assembly files
    {
        std::ofstream f(x86_neoa);
        f << "origin(SYSCALL)\n";
    }
    {
        std::ofstream f(arm_neoa);
        f << "origin(SVC 0)\n";
    }

    // 2. Compile to flat binary using compile_file
    assert(NeoCompiler::compile_file(x86_neoa, x86_bin, "x86_64"));
    assert(NeoCompiler::compile_file(arm_neoa, arm_bin, "aarch64"));

    const auto x86_bytes = read_bin_file(x86_bin);
    const auto arm_bytes = read_bin_file(arm_bin);

    // 3. Verify exactly matching byte size and no container headers
    assert(x86_bytes.size() == 2 && x86_bytes[0] == 0x0F && x86_bytes[1] == 0x05);
    assert(!contains_container_headers(x86_bytes));

    assert(arm_bytes.size() == 4 && arm_bytes[0] == 0xD4 && arm_bytes[3] == 0x01);
    assert(!contains_container_headers(arm_bytes));

    std::filesystem::remove(x86_neoa);
    std::filesystem::remove(x86_bin);
    std::filesystem::remove(arm_neoa);
    std::filesystem::remove(arm_bin);
}

void test_preprocessor_conditionals_and_includes(const std::filesystem::path&) {
    const auto temp_dir = std::filesystem::temp_directory_path();
    const auto inc_path = temp_dir / "header_inc.neoa";
    const auto main_path = temp_dir / "main_inc.neoa";
    const auto out_path = temp_dir / "out_inc.bin";

    {
        std::ofstream inc_file(inc_path);
        inc_file << "origin(SYSCALL)\n";
    }
    {
        std::ofstream main_file(main_path);
        main_file << "@IF __arch__ == \"x86_64\"\n"
                  << "@INCLUDE \"" << inc_path.filename().string() << "\"\n"
                  << "@ENDIF\n";
    }

    assert(NeoCompiler::compile_file(main_path, out_path, "x86_64"));
    const auto bytes = read_bin_file(out_path);
    assert(bytes.size() == 2 && bytes[0] == 0x0F && bytes[1] == 0x05);

    std::filesystem::remove(inc_path);
    std::filesystem::remove(main_path);
    std::filesystem::remove(out_path);
}

void test_error_handling(const std::filesystem::path& tmpl_dir) {
    std::vector<uint8_t> bin;

    // 1. Missing file error handling
    assert(!NeoCompiler::compile_file("non_existent_file.neoa", "out.bin", "x86_64"));
    assert(!NeoCompiler::last_errors().empty());

    // 2. Syntax error handling
    const auto db_path = std::filesystem::temp_directory_path() / "test_asm_x86.neodb";
    if (!std::filesystem::exists(db_path)) {
        assert(NeoTranslator::yaml_to_neodb(tmpl_dir / "neo_asm_x86_64.yaml", db_path));
    }
    NeoDatabase db;
    assert(db.load_file(db_path));

    assert(!NeoCompiler::compile_source("INVALID_OPCODE_XYZ r0, r1\n", db, bin));
    assert(!NeoCompiler::last_errors().empty());
}

} // namespace

void test_suite_pipeline(const std::filesystem::path& tmpl_dir) {
    test_flat_binary_no_headers(tmpl_dir);
    test_preprocessor_conditionals_and_includes(tmpl_dir);
    test_error_handling(tmpl_dir);
    std::cout << "[PASS] Flat binary pipeline and preprocessor suite\n";
}

} // namespace neo::test
