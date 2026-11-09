/**
 * @file test_preprocessor.cpp
 * @brief Test runner for Neo-ASM preprocessor test suite.
 */

#include "test_preprocessor_suites.hpp"

#include <iostream>

int main() {
    std::cout << "Running Neo-ASM Preprocessor test suite...\n";

    neo::test::run_conditional_tests();
    neo::test::run_include_tests();

    std::cout << "All Neo-ASM Preprocessor tests passed successfully!\n";
    return 0;
}
