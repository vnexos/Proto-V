/**
 * @file test_parser_labels.cpp
 * @brief Unit tests for hierarchical indentation-based label resolution.
 */

#include "test_parser_suites.hpp"
#include "neo/parser.hpp"

#include <cassert>
#include <iostream>

namespace neo::test {

namespace {

void test_hierarchical_scopes() {
    const std::string source =
        "label_a:\n"
        "    ASN r0, 1\n"
        "    label_b:\n"
        "        ASN r1, 2\n"
        "        label_c:\n"
        "            ASN r2, 3\n"
        "            JMP label_c\n"
        "            JMP label_b\n"
        "            JMP label_a\n"
        "            JMP label_a.label_b.label_c\n"
        "    label_d:\n"
        "        ASN r3, 4\n"
        "        JMP label_b.label_c\n";

    const auto res = NeoParser::parse(source);
    for (const auto& err : res.errors) {
        std::cerr << "Label test error: " << err << "\n";
    }
    assert(res.success && res.instructions.size() == 9);

    // Verify symbol entries
    const auto* sym_a = res.symbols.find_exact("label_a");
    const auto* sym_b = res.symbols.find_exact("label_a.label_b");
    const auto* sym_c = res.symbols.find_exact("label_a.label_b.label_c");
    const auto* sym_d = res.symbols.find_exact("label_a.label_d");

    assert(sym_a != nullptr && sym_a->instruction_index == 0);
    assert(sym_b != nullptr && sym_b->instruction_index == 1);
    assert(sym_c != nullptr && sym_c->instruction_index == 2);
    assert(sym_d != nullptr && sym_d->instruction_index == 7);

    // Instruction 3: JMP label_c inside scope label_a.label_b.label_c
    assert(res.instructions[3].operands[0].resolved_label == "label_a.label_b.label_c");
    // Instruction 4: JMP label_b inside scope label_a.label_b.label_c
    assert(res.instructions[4].operands[0].resolved_label == "label_a.label_b");
    // Instruction 5: JMP label_a inside scope label_a.label_b.label_c
    assert(res.instructions[5].operands[0].resolved_label == "label_a");
    // Instruction 6: JMP label_a.label_b.label_c (fully qualified)
    assert(res.instructions[6].operands[0].resolved_label == "label_a.label_b.label_c");
    // Instruction 8: JMP label_b.label_c inside label_a.label_d
    assert(res.instructions[8].operands[0].resolved_label == "label_a.label_b.label_c");
}

void test_deeply_nested_labels() {
    const std::string source =
        "l1:\n"
        "    l2:\n"
        "        l3:\n"
        "            l4:\n"
        "                l5:\n"
        "                    ASN r0, r0\n"
        "                    JMP l5\n"
        "                    JMP l1\n";

    const auto res = NeoParser::parse(source);
    assert(res.success && res.instructions.size() == 3);
    assert(res.symbols.contains("l1.l2.l3.l4.l5"));
    assert(res.instructions[1].operands[0].resolved_label == "l1.l2.l3.l4.l5");
    assert(res.instructions[2].operands[0].resolved_label == "l1");
}

void test_tab_indentation() {
    const std::string source =
        "root:\n"
        "\tchild:\n"
        "\t\tASN r0, 42\n"
        "\t\tJMP child\n";

    const auto res = NeoParser::parse(source);
    assert(res.success && res.instructions.size() == 2);
    assert(res.symbols.contains("root.child"));
    assert(res.instructions[1].operands[0].resolved_label == "root.child");
}

} // namespace

void run_label_tests() {
    test_hierarchical_scopes();
    test_deeply_nested_labels();
    test_tab_indentation();
    std::cout << "[PASS] Hierarchical label test suite\n";
}

} // namespace neo::test
