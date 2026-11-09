/**
 * @file test_cli_workflows.hpp
 * @brief Declarations of end-to-end integration and compilation workflows for neo-asm CLI.
 */

#pragma once

#include <filesystem>
#include <string>

namespace neo::test {

/**
 * @brief Executes end-to-end workflow integration tests for neo-asm CLI.
 *
 * Verifies that:
 * - Database installation (-idb and --install-database) copies .neodb to ./database/
 * - Compilation automatically resolves .neodb from ./database without --db flag
 * - Directives (@INCLUDE, @IF, @IFNDEF) are evaluated cleanly via CLI compilation
 * - Multi-architecture sample assembly (x86_64, AArch64, RISC-V 64) emits pure machine code
 * - Lossless roundtrip (YAML -> neodb -> YAML -> neodb) produces bit-identical binaries
 *
 * @param exe Path to neo-asm executable binary.
 * @param tmpl_dir Path to reference YAML templates directory.
 */
void run_cli_workflow_tests(const std::string& exe, const std::filesystem::path& tmpl_dir);

} // namespace neo::test
