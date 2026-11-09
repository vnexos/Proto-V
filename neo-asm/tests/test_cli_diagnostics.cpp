/**
 * @file test_cli_diagnostics.cpp
 * @brief Diagnostic error reporting and exit code assertions for neo-asm CLI.
 */

#include "test_cli_diagnostics.hpp"
#include "test_cli_process.hpp"

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace neo::test {

namespace {

void test_no_arguments(const std::string& exe) {
    const auto res = run_process(exe);
    assert(res.exit_code == 1);
    assert(res.stderr_output.find("neo-asm") != std::string::npos ||
           res.stderr_output.find("Usage:") != std::string::npos ||
           res.stderr_output.find("OPTIONS:") != std::string::npos);
    std::cout << "[PASS] test_no_arguments\n";
}

void test_invalid_option(const std::string& exe) {
    const auto res = run_process(exe + " --invalid-option-flag");
    assert(res.exit_code == 1);
    assert(res.stderr_output.find("Error:") != std::string::npos);
    std::cout << "[PASS] test_invalid_option\n";
}

void test_missing_files(const std::string& exe) {
    const auto res_tr = run_process(exe + " -t non_existent.yaml /tmp/out.neodb");
    assert(res_tr.exit_code == 1);
    assert(res_tr.stderr_output.find("does not exist") != std::string::npos);

    const auto res_idb = run_process(exe + " -idb non_existent.neodb");
    assert(res_idb.exit_code == 1);
    assert(res_idb.stderr_output.find("does not exist") != std::string::npos);
    std::cout << "[PASS] test_missing_files\n";
}

void test_corrupt_database(const std::string& exe) {
    const auto corrupt_path = std::filesystem::temp_directory_path() / "test_corrupt_crc.neodb";

    // Write a dummy 64-byte header with valid magic but deliberately corrupted CRC32
    std::vector<uint8_t> dummy_bytes(64, 0);
    std::memcpy(dummy_bytes.data(), "NEODB\0\0\0", 8);
    // Format version 0x00010000
    dummy_bytes[8] = 0x00;
    dummy_bytes[9] = 0x00;
    dummy_bytes[10] = 0x01;
    dummy_bytes[11] = 0x00;
    // Corrupt CRC32 at byte 60
    dummy_bytes[60] = 0xAA;
    dummy_bytes[61] = 0xBB;
    dummy_bytes[62] = 0xCC;
    dummy_bytes[63] = 0xDD;

    {
        std::ofstream out(corrupt_path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(dummy_bytes.data()), dummy_bytes.size());
    }

    const auto res = run_process(exe + " -t \"" + corrupt_path.string() + "\" /tmp/corrupt_out.yaml");
    std::filesystem::remove(corrupt_path);

    assert(res.exit_code == 1);
    assert(res.stderr_output.find("CRC32") != std::string::npos ||
           res.stderr_output.find("checksum mismatch") != std::string::npos);
    std::cout << "[PASS] test_corrupt_database\n";
}

void test_syntax_errors(const std::string& exe) {
    const auto bad_neoa = std::filesystem::temp_directory_path() / "test_syntax_err.neoa";
    {
        std::ofstream out(bad_neoa);
        out << "; Neo-ASM with syntax error on line 2\n";
        out << "# invalid comment syntax at line 2\n";
    }

    const auto res = run_process(exe + " -c \"" + bad_neoa.string() + "\" /tmp/syntax.bin --arch=x86_64");
    std::filesystem::remove(bad_neoa);

    assert(res.exit_code == 1);
    assert(res.stderr_output.find("line 2") != std::string::npos);
    std::cout << "[PASS] test_syntax_errors\n";
}

} // namespace

void run_cli_diagnostics_tests(const std::string& exe) {
    test_no_arguments(exe);
    test_invalid_option(exe);
    test_missing_files(exe);
    test_corrupt_database(exe);
    test_syntax_errors(exe);
}

} // namespace neo::test
