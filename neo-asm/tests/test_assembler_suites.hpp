/**
 * @file test_assembler_suites.hpp
 * @brief Declarations of modular test suites for neo-asm integration testing.
 */

#pragma once

#include <filesystem>

namespace neo::test {

/**
 * @brief Verifies raw arithmetic machine code against NASM and GNU as references.
 */
void test_suite_arithmetic(const std::filesystem::path& tmpl_dir);

/**
 * @brief Verifies native passthrough instructions (SYSCALL, SVC, ECALL).
 */
void test_suite_origin(const std::filesystem::path& tmpl_dir);

/**
 * @brief Verifies flat binary output, absence of container headers, and preprocessor.
 */
void test_suite_pipeline(const std::filesystem::path& tmpl_dir);

/**
 * @brief Verifies position-independent code (PIC) branch tags and x86_64 spills.
 */
void test_suite_spill_and_pic(const std::filesystem::path& tmpl_dir);

} // namespace neo::test
