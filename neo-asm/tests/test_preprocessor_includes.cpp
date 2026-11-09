/**
 * @file test_preprocessor_includes.cpp
 * @brief Unit tests for @INCLUDE file resolution and preprocessor diagnostics.
 */

#include "test_preprocessor_suites.hpp"
#include <neo/preprocessor.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace neo::test {

namespace {

struct TempDirFixture {
    std::filesystem::path dir;
    TempDirFixture() {
        dir = std::filesystem::temp_directory_path() / ("neo_test_" + std::to_string(std::clock()));
        std::filesystem::create_directories(dir);
    }
    ~TempDirFixture() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
    void write_file(const std::string& name, std::string_view content) const {
        std::ofstream f(dir / name, std::ios::out | std::ios::binary);
        f << content;
    }
};

void test_include_appends_to_end() {
    TempDirFixture fixture;
    fixture.write_file("sub.neoa", "helper:\n    RET\n");
    fixture.write_file("main.neoa", "ASN r0, 1\n@INCLUDE \"sub.neoa\"\nASN r1, 2\n");

    NeoPreprocessor pp;
    auto r = pp.process_file(fixture.dir / "main.neoa");
    assert(r.success);

    const std::string expected = "ASN r0, 1\nASN r1, 2\nhelper:\n    RET\n";
    assert(r.output_text == expected);
    assert(r.output_text.find("@INCLUDE") == std::string::npos);

    std::cout << "[PASS] test_include_appends_to_end\n";
}

void test_multiple_and_nested_includes() {
    TempDirFixture fixture;
    fixture.write_file("c.neoa", "sub_c:\n    RET\n");
    fixture.write_file("b.neoa", "sub_b:\n    @INCLUDE \"c.neoa\"\n    NOP\n");
    fixture.write_file("a.neoa", "start:\n    @INCLUDE \"b.neoa\"\n    ASN r0, 42\n");

    NeoPreprocessor pp;
    auto r = pp.process_file(fixture.dir / "a.neoa");
    assert(r.success);

    const std::string expected = "start:\n    ASN r0, 42\nsub_b:\n    NOP\nsub_c:\n    RET\n";
    assert(r.output_text == expected);

    std::cout << "[PASS] test_multiple_and_nested_includes\n";
}

void test_circular_include_detected() {
    TempDirFixture fixture;
    fixture.write_file("circ_a.neoa", "ASN r0, 1\n@INCLUDE \"circ_b.neoa\"\n");
    fixture.write_file("circ_b.neoa", "ASN r1, 2\n@INCLUDE \"circ_a.neoa\"\n");

    NeoPreprocessor pp;
    auto r = pp.process_file(fixture.dir / "circ_a.neoa");
    assert(!r.success);
    assert(!r.errors.empty());

    bool found_circ = false;
    for (const auto& err : r.errors) {
        if (err.find("Circular inclusion detected") != std::string::npos) found_circ = true;
    }
    assert(found_circ);

    std::cout << "[PASS] test_circular_include_detected\n";
}

void test_missing_include_error() {
    NeoPreprocessor pp;
    const std::string_view src = "ASN r0, 1\n@INCLUDE \"non_existent_file.neoa\"\n";
    auto r = pp.process(src, "test_input.neoa");
    assert(!r.success);
    assert(!r.errors.empty());

    const std::string& err = r.errors[0];
    assert(err.find("test_input.neoa:2:") != std::string::npos);
    assert(err.find("Include file not found") != std::string::npos);

    std::cout << "[PASS] test_missing_include_error\n";
}

void test_unclosed_if_error() {
    NeoPreprocessor pp;
    const std::string_view src = "@IF __os__ == \"linux\"\n    ASN r0, 1\n";
    auto r = pp.process(src, "my_file.neoa");
    assert(!r.success);
    assert(!r.errors.empty());

    const std::string& err = r.errors[0];
    assert(err.find("my_file.neoa:1:") != std::string::npos);
    assert(err.find("Unclosed @IF directive") != std::string::npos);

    std::cout << "[PASS] test_unclosed_if_error\n";
}

void test_unmatched_directives_errors() {
    NeoPreprocessor pp;
    auto r_else = pp.process("@ELSE\n", "err.neoa");
    assert(!r_else.success && r_else.errors[0].find("Unmatched @ELSE") != std::string::npos);

    auto r_endif = pp.process("@ENDIF\n", "err.neoa");
    assert(!r_endif.success && r_endif.errors[0].find("Unmatched @ENDIF") != std::string::npos);

    auto r_dup = pp.process("@IF 1\n@ELSE\n@ELSE\n@ENDIF\n", "err.neoa");
    assert(!r_dup.success && r_dup.errors[0].find("Duplicate @ELSE") != std::string::npos);

    std::cout << "[PASS] test_unmatched_directives_errors\n";
}

} // namespace

void run_include_tests() {
    test_include_appends_to_end();
    test_multiple_and_nested_includes();
    test_circular_include_detected();
    test_missing_include_error();
    test_unclosed_if_error();
    test_unmatched_directives_errors();
}

} // namespace neo::test
