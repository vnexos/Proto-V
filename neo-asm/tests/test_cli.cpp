/**
 * @file test_cli.cpp
 * @brief Integration tests for neo-asm command-line executable.
 */

#include "test_cli_diagnostics.hpp"
#include "test_cli_workflows.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <vector>

#ifndef NEO_TEMPLATES_DIR
#define NEO_TEMPLATES_DIR "../../../ai-driven/templates"
#endif

#ifndef NEO_ASM_EXE
#define NEO_ASM_EXE "./neo-asm"
#endif

namespace neo::test {

namespace {

/**
 * @brief Locates the directory containing reference architecture YAML definitions.
 * @return Resolved filesystem path to templates directory.
 */
std::filesystem::path resolve_templates_dir() {
    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::path(NEO_TEMPLATES_DIR),
        std::filesystem::path("/home/tvdquang/Code/ai-driven/templates"),
        std::filesystem::current_path() / "ai-driven/templates",
        std::filesystem::current_path() / "../../../ai-driven/templates",
        std::filesystem::current_path() / "../../ai-driven/templates"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate / "neo_asm_x86_64.yaml")) {
            return candidate;
        }
    }
    return {};
}

} // namespace

} // namespace neo::test

int main() {
    const std::string exe = NEO_ASM_EXE;
    const auto tmpl_dir = neo::test::resolve_templates_dir();
    assert(!tmpl_dir.empty());

    std::cout << "Starting neo-asm CLI diagnostic and error assertions...\n";
    neo::test::run_cli_diagnostics_tests(exe);

    std::cout << "\nStarting neo-asm CLI workflow and multi-architecture assertions...\n";
    neo::test::run_cli_workflow_tests(exe, tmpl_dir);

    std::cout << "\nAll neo-asm CLI integration tests passed successfully.\n";
    return 0;
}
