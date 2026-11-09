/**
 * @file cli_commands.hpp
 * @brief Command execution routines for the neo-asm CLI driver.
 */

#pragma once

#include <filesystem>
#include <string_view>

namespace neo::cli {

/**
 * @brief Resolves target architecture database path from candidate locations.
 *
 * Checks in order:
 * 1. Explicit override path (--db=<path>)
 * 2. <exe_dir>/database/<arch>.neodb (relative to executable directory)
 * 3. ./database/<arch>.neodb (relative to current working directory)
 * 4. ./<arch>.neodb (current working directory)
 *
 * @param arch Target architecture name (e.g. "x86_64", "aarch64", "riscv64").
 * @param custom_db Optional explicit database override path.
 * @param exe_dir Directory containing the running executable.
 * @return Resolved filesystem path to .neodb, or empty path if not found.
 */
std::filesystem::path resolve_database_path(
    std::string_view arch,
    const std::filesystem::path& custom_db,
    const std::filesystem::path& exe_dir);

/**
 * @brief Executes database installation into target database directories.
 *
 * Validates file existence and header/CRC32 integrity of the source .neodb file.
 * Creates target ./database/ directory (and <exe_dir>/database/ if applicable)
 * and copies the database file.
 *
 * @param db_path Path to the source .neodb file.
 * @param exe_dir Directory containing the running executable.
 * @return 0 on success, 1 on failure.
 */
int execute_install_database(
    const std::filesystem::path& db_path,
    const std::filesystem::path& exe_dir);

/**
 * @brief Executes bidirectional translation between YAML and .neodb formats.
 *
 * Auto-detects translation direction based on input file extension:
 * - .yaml / .yml -> compiles to .neodb
 * - .neodb -> validates header/CRC32 and decompiles to YAML
 *
 * @param in_path Input specification or database file path.
 * @param out_path Output file path.
 * @return 0 on success, 1 on failure.
 */
int execute_translation(
    const std::filesystem::path& in_path,
    const std::filesystem::path& out_path);

/**
 * @brief Executes compilation of Neo-ASM source (.neoa) into flat machine code (.bin).
 *
 * @param in_path Path to input .neoa source file.
 * @param out_path Path to output .bin destination file.
 * @param arch Target architecture name (e.g. "x86_64", "aarch64", "riscv64").
 * @param custom_db Optional explicit database override path.
 * @param exe_dir Directory containing the running executable.
 * @return 0 on success, 1 on failure.
 */
int execute_compilation(
    const std::filesystem::path& in_path,
    const std::filesystem::path& out_path,
    std::string_view arch,
    const std::filesystem::path& custom_db,
    const std::filesystem::path& exe_dir);

} // namespace neo::cli
