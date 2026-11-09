/**
 * @file test_assembler.cpp
 * @brief Master test runner for Neo-ASM multi-pass assembler integration test suite.
 */

#include "test_assembler_suites.hpp"

#include <filesystem>
#include <iostream>
#include <vector>

#ifndef NEO_TEMPLATES_DIR
#define NEO_TEMPLATES_DIR "../../../ai-driven/templates"
#endif

namespace {

std::filesystem::path resolve_templates_dir() {
    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::path(NEO_TEMPLATES_DIR),
        std::filesystem::path("/home/tvdquang/Code/ai-driven/templates"),
        std::filesystem::current_path() / "ai-driven/templates",
        std::filesystem::current_path() / "../../ai-driven/templates",
        std::filesystem::current_path() / "../../../ai-driven/templates"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate / "neo_asm_x86_64.yaml")) {
            return candidate;
        }
    }
    return {};
}

} // namespace

int main() {
    std::cout << "[TEST] Running test_assembler suite..." << std::endl;

    const auto tmpl_dir = resolve_templates_dir();
    if (tmpl_dir.empty()) {
        std::cerr << "[ERROR] Could not locate architecture templates directory" << std::endl;
        return 1;
    }

    neo::test::test_suite_arithmetic(tmpl_dir);
    neo::test::test_suite_origin(tmpl_dir);
    neo::test::test_suite_pipeline(tmpl_dir);
    neo::test::test_suite_spill_and_pic(tmpl_dir);

    std::cout << "[PASS] All assembler test suites completed successfully!" << std::endl;
    return 0;
}
