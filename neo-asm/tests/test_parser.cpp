/**
 * @file test_parser.cpp
 * @brief Master test runner for Neo-ASM Assembly Lexer and AST Parser.
 */

#include "test_parser_suites.hpp"

#include <iostream>

int main() {
    std::cout << "Running Neo-ASM Lexer and AST Parser test suite...\n";

    neo::test::run_instruction_tests();
    neo::test::run_label_tests();
    neo::test::run_error_tests();

    std::cout << "All Neo-ASM Lexer and AST Parser tests passed successfully!\n";
    return 0;
}
