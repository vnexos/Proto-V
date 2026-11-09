/**
 * @file test_parser_errors.cpp
 * @brief Unit tests for comments, error diagnostic reporting, and syntax validation.
 */

#include "test_parser_suites.hpp"
#include "neo/parser.hpp"

#include <cassert>
#include <iostream>

namespace neo::test {

namespace {

void test_comments_parsing() {
    const std::string source =
        "; Leading semicolon comment\n"
        "/* Inline block comment */ ASN r0, r1 ; Trailing semicolon comment\n"
        "/* Multi-line\n"
        "   block comment */\n"
        "ADD r2, r3, r4\n";

    const auto res = NeoParser::parse(source);
    assert(res.success && res.instructions.size() == 2);
    assert(res.instructions[0].mnemonic == "ASN");
    assert(res.instructions[1].mnemonic == "ADD");
}

void test_rejected_comment_syntaxes() {
    // Rejects '#' syntax
    const auto res_hash = NeoParser::parse("# This comment should fail\nASN r0, r1\n");
    assert(!res_hash.success && !res_hash.errors.empty());
    assert(res_hash.errors[0].find("line") != std::string::npos);
    assert(res_hash.errors[0].find("column") != std::string::npos);

    // Rejects '//' syntax
    const auto res_slash = NeoParser::parse("// This comment should fail\nASN r0, r1\n");
    assert(!res_slash.success && !res_slash.errors.empty());
    assert(res_slash.errors[0].find("line") != std::string::npos);
    assert(res_slash.errors[0].find("column") != std::string::npos);
}

void test_missing_comma_diagnostics() {
    const auto res = NeoParser::parse("ASN r0 r1\n");
    assert(!res.success && !res.errors.empty());
    assert(res.errors[0].find("Expected ','") != std::string::npos);
    assert(res.errors[0].find("line") != std::string::npos);
    assert(res.errors[0].find("column") != std::string::npos);
}

void test_invalid_register_diagnostics() {
    const auto res = NeoParser::parse("ASN r32, r0\n");
    assert(!res.success && !res.errors.empty());
    assert(res.errors[0].find("Invalid register 'r32'") != std::string::npos);
    assert(res.errors[0].find("line") != std::string::npos);
    assert(res.errors[0].find("column") != std::string::npos);
}

void test_unclosed_memory_bracket_diagnostics() {
    const auto res = NeoParser::parse("ASN r0, [r1 + 8\n");
    assert(!res.success && !res.errors.empty());
    assert(res.errors[0].find("Unclosed memory bracket ']'") != std::string::npos);
    assert(res.errors[0].find("line") != std::string::npos);
    assert(res.errors[0].find("column") != std::string::npos);
}

void test_unclosed_block_comment_diagnostics() {
    const auto res = NeoParser::parse("/* unclosed comment\nASN r0, r1\n");
    // With unclosed block comment, EOF is reached before */
    assert(res.instructions.empty());
}

} // namespace

void run_error_tests() {
    test_comments_parsing();
    test_rejected_comment_syntaxes();
    test_missing_comma_diagnostics();
    test_invalid_register_diagnostics();
    test_unclosed_memory_bracket_diagnostics();
    test_unclosed_block_comment_diagnostics();
    std::cout << "[PASS] Parser diagnostics and error test suite\n";
}

} // namespace neo::test
