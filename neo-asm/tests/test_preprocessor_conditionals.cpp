/**
 * @file test_preprocessor_conditionals.cpp
 * @brief Unit tests for conditional directives (@IF, @IFNDEF, @ELSE, @ENDIF).
 */

#include "test_preprocessor_suites.hpp"
#include <neo/preprocessor.hpp>

#include <cassert>
#include <iostream>

namespace neo::test {

namespace {

void test_os_conditionals() {
    PreprocessorContext win_ctx;
    win_ctx.target_os = "windows";
    win_ctx.target_arch = "x86_64";

    PreprocessorContext linux_ctx;
    linux_ctx.target_os = "linux";
    linux_ctx.target_arch = "x86_64";

    const std::string_view src_win =
        "@IF __os__ == \"windows\"\n"
        "    ASN r0, 10\n"
        "@ENDIF\n";

    auto win_on_win = NeoPreprocessor::run(src_win, "", win_ctx);
    assert(win_on_win.success);
    assert(win_on_win.output_text == "    ASN r0, 10\n");

    auto win_on_linux = NeoPreprocessor::run(src_win, "", linux_ctx);
    assert(win_on_linux.success);
    assert(win_on_linux.output_text.empty());

    const std::string_view src_linux =
        "@IF __os__ == \"linux\"\n"
        "    ASN r0, 20\n"
        "@ENDIF\n";

    auto linux_on_linux = NeoPreprocessor::run(src_linux, "", linux_ctx);
    assert(linux_on_linux.success);
    assert(linux_on_linux.output_text == "    ASN r0, 20\n");

    auto linux_on_win = NeoPreprocessor::run(src_linux, "", win_ctx);
    assert(linux_on_win.success);
    assert(linux_on_win.output_text.empty());

    std::cout << "[PASS] test_os_conditionals\n";
}

void test_arch_conditionals() {
    PreprocessorContext x86_ctx;
    x86_ctx.target_arch = "x86_64";
    PreprocessorContext arm_ctx;
    arm_ctx.target_arch = "aarch64";
    PreprocessorContext rv_ctx;
    rv_ctx.target_arch = "riscv64";

    const std::string_view src =
        "@IF __arch__ == \"x86_64\"\n"
        "    ASN r0, 1\n"
        "@ELIF __arch__ == \"aarch64\"\n"
        "    ASN r0, 2\n"
        "@ELSE\n"
        "    ASN r0, 3\n"
        "@ENDIF\n";

    auto r_x86 = NeoPreprocessor::run(src, "", x86_ctx);
    assert(r_x86.success && r_x86.output_text == "    ASN r0, 1\n");

    auto r_arm = NeoPreprocessor::run(src, "", arm_ctx);
    assert(r_arm.success && r_arm.output_text == "    ASN r0, 2\n");

    auto r_rv = NeoPreprocessor::run(src, "", rv_ctx);
    assert(r_rv.success && r_rv.output_text == "    ASN r0, 3\n");

    std::cout << "[PASS] test_arch_conditionals\n";
}

void test_ifndef_and_ifdef() {
    PreprocessorContext ctx;

    const std::string_view src_ifndef =
        "@IFNDEF MY_SYMBOL\n"
        "    ASN r0, 100\n"
        "@ENDIF\n";

    auto r1 = NeoPreprocessor::run(src_ifndef, "", ctx);
    assert(r1.success);
    assert(r1.output_text == "    ASN r0, 100\n");

    ctx.symbols["MY_SYMBOL"] = "1";
    auto r2 = NeoPreprocessor::run(src_ifndef, "", ctx);
    assert(r2.success);
    assert(r2.output_text.empty());

    const std::string_view src_ifdef =
        "@IFDEF MY_SYMBOL\n"
        "    ASN r0, 200\n"
        "@ENDIF\n";

    auto r3 = NeoPreprocessor::run(src_ifdef, "", ctx);
    assert(r3.success && r3.output_text == "    ASN r0, 200\n");

    ctx.symbols.erase("MY_SYMBOL");
    auto r4 = NeoPreprocessor::run(src_ifdef, "", ctx);
    assert(r4.success && r4.output_text.empty());

    std::cout << "[PASS] test_ifndef_and_ifdef\n";
}

void test_nested_conditionals() {
    PreprocessorContext ctx;
    ctx.target_os = "linux";
    ctx.target_arch = "x86_64";

    const std::string_view src =
        "@IF __os__ == \"linux\"\n"
        "    @IF __arch__ == \"x86_64\"\n"
        "        ASN r0, 1\n"
        "    @ELSE\n"
        "        ASN r0, 2\n"
        "    @ENDIF\n"
        "@ELSE\n"
        "    ASN r0, 3\n"
        "@ENDIF\n";

    auto r = NeoPreprocessor::run(src, "", ctx);
    assert(r.success);
    assert(r.output_text == "        ASN r0, 1\n");
    assert(r.output_text.find("@IF") == std::string::npos);
    assert(r.output_text.find("@ELSE") == std::string::npos);
    assert(r.output_text.find("@ENDIF") == std::string::npos);

    ctx.target_arch = "aarch64";
    auto r_arm = NeoPreprocessor::run(src, "", ctx);
    assert(r_arm.success && r_arm.output_text == "        ASN r0, 2\n");

    ctx.target_os = "windows";
    auto r_win = NeoPreprocessor::run(src, "", ctx);
    assert(r_win.success && r_win.output_text == "    ASN r0, 3\n");

    std::cout << "[PASS] test_nested_conditionals\n";
}

void test_user_defines_and_logic() {
    PreprocessorContext ctx;
    NeoPreprocessor pp(ctx);

    const std::string_view src =
        "@DEFINE FEATURE_A 1\n"
        "@DEFINE FEATURE_B 0\n"
        "@IF defined(FEATURE_A) && !defined(FEATURE_C)\n"
        "    ASN r0, 42\n"
        "@ENDIF\n"
        "@UNDEF FEATURE_A\n"
        "@IFNDEF FEATURE_A\n"
        "    ASN r1, 84\n"
        "@ENDIF\n";

    auto r = pp.process(src);
    assert(r.success);
    assert(r.output_text == "    ASN r0, 42\n    ASN r1, 84\n");

    std::cout << "[PASS] test_user_defines_and_logic\n";
}

} // namespace

void run_conditional_tests() {
    test_os_conditionals();
    test_arch_conditionals();
    test_ifndef_and_ifdef();
    test_nested_conditionals();
    test_user_defines_and_logic();
}

} // namespace neo::test
