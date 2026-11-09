/**
 * @file test_cli_diagnostics.hpp
 * @brief Diagnostic and error handling test suite declarations for neo-asm CLI.
 */

#pragma once

#include <string>

namespace neo::test {

/**
 * @brief Executes diagnostic and failure path assertions for neo-asm CLI.
 *
 * Verifies that:
 * - Invocation without arguments outputs usage and halts with code 1
 * - Unrecognized flags output error diagnostics and halt with code 1
 * - Non-existent input files output descriptive diagnostics and halt with code 1
 * - Corrupted databases with CRC32 mismatch halt with code 1
 * - Assembly syntax errors output line numbers and halt with code 1
 *
 * @param exe Path to neo-asm executable binary.
 */
void run_cli_diagnostics_tests(const std::string& exe);

} // namespace neo::test
