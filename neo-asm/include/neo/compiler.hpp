/**
 * @file compiler.hpp
 * @brief Multi-pass compiler orchestrator compiling Neo-ASM (.neoa) into flat binaries (.bin).
 */

#pragma once

#include "neo/database.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace neo {

/**
 * @brief Multi-pass compilation pipeline for Neo-ASM source code.
 *
 * Coordinates:
 * - Pass 0: Preprocessor (@INCLUDE, @IF, @IFNDEF, conditional directives)
 * - Pass 1: Assembly Lexer and AST Parser
 * - Pass 2: Architecture Expansion, Register Virtualization, and x86_64 Stack Spilling
 * - Pass 3: Position-Independent Code (PIC) Relative Branch Displacement Resolution
 * - Pass 4: Bitfield Slot Masking, Shifting, and Flat Binary Machine Code Emission
 */
class NeoCompiler {
public:
    /**
     * @brief Compiles a Neo-ASM assembly source file to a pure flat machine code binary file.
     *
     * Automatically resolves and loads the target architecture database (.neodb).
     *
     * @param in_neoa Path to input .neoa source file.
     * @param out_bin Path to output .bin destination file.
     * @param arch Target architecture name (e.g. "x86_64", "aarch64", "riscv64").
     * @param db_path Optional explicit path to .neodb file.
     * @return True if compilation succeeded, false on error.
     */
    static bool compile_file(
        const std::filesystem::path& in_neoa,
        const std::filesystem::path& out_bin,
        std::string_view arch,
        const std::filesystem::path& db_path = "");

    /**
     * @brief Compiles a Neo-ASM assembly source file using an already loaded NeoDatabase.
     * @param in_neoa Path to input .neoa source file.
     * @param out_bin Path to output .bin destination file.
     * @param db Loaded and validated NeoDatabase instance.
     * @return True if compilation succeeded, false on error.
     */
    static bool compile_file(
        const std::filesystem::path& in_neoa,
        const std::filesystem::path& out_bin,
        const NeoDatabase& db);

    /**
     * @brief Compiles raw Neo-ASM source code into a pure flat machine bytecode vector.
     * @param source Neo-ASM source code text.
     * @param db Loaded and validated NeoDatabase instance.
     * @param out_bin Output byte vector where machine code will be written.
     * @param current_file Optional path of current source file (for relative @INCLUDEs).
     * @return True if compilation succeeded, false on error.
     */
    static bool compile_source(
        std::string_view source,
        const NeoDatabase& db,
        std::vector<uint8_t>& out_bin,
        const std::filesystem::path& current_file = "");

    /**
     * @brief Returns diagnostics and error messages produced by the most recent compilation.
     * @return Const reference to vector of error strings.
     */
    [[nodiscard]] static const std::vector<std::string>& last_errors() noexcept;
};

} // namespace neo
