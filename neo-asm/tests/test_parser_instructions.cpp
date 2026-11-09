/**
 * @file test_parser_instructions.cpp
 * @brief Unit tests for Neo-ASM instruction parsing.
 */

#include "test_parser_suites.hpp"
#include "neo/parser.hpp"

#include <cassert>
#include <iostream>

namespace neo::test {

namespace {

void test_register_assign() {
    const auto res = NeoParser::parse("ASN r0, r1\n");
    assert(res.success && res.instructions.size() == 1);
    const auto& inst = res.instructions[0];
    assert(inst.mnemonic == "ASN");
    assert(inst.operands.size() == 2);
    assert(inst.operands[0].kind == OperandKind::Register && inst.operands[0].reg == 0);
    assert(inst.operands[1].kind == OperandKind::Register && inst.operands[1].reg == 1);
}

void test_memory_load_store() {
    const auto res = NeoParser::parse(
        "ASN r0, [r1 + 8]\n"
        "ASN [r1 + 8], r0\n"
        "ASN r2, [sp + 16]\n"
        "ASN [fp - 24], r3\n"
        "ASN r4, [r5]\n"
    );
    assert(res.success && res.instructions.size() == 5);
    // Load
    assert(res.instructions[0].operands[0].kind == OperandKind::Register && res.instructions[0].operands[0].reg == 0);
    assert(res.instructions[0].operands[1].kind == OperandKind::Memory);
    assert(res.instructions[0].operands[1].mem.base_reg == 1 && res.instructions[0].operands[1].mem.displacement == 8);
    // Store
    assert(res.instructions[1].operands[0].kind == OperandKind::Memory);
    assert(res.instructions[1].operands[0].mem.base_reg == 1 && res.instructions[1].operands[0].mem.displacement == 8);
    assert(res.instructions[1].operands[1].kind == OperandKind::Register && res.instructions[1].operands[1].reg == 0);
    // System register SP load
    assert(res.instructions[2].operands[1].mem.base_reg == REG_SP && res.instructions[2].operands[1].mem.is_base_system);
    // System register FP store with negative displacement
    assert(res.instructions[3].operands[0].mem.base_reg == REG_FP && res.instructions[3].operands[0].mem.displacement == -24);
    // Zero displacement
    assert(res.instructions[4].operands[1].mem.base_reg == 5 && res.instructions[4].operands[1].mem.displacement == 0);
}

void test_stack_operations() {
    const auto res = NeoParser::parse("PUSH r0\nPOP r1\n");
    assert(res.success && res.instructions.size() == 2);
    assert(res.instructions[0].mnemonic == "PUSH" && res.instructions[0].operands[0].reg == 0);
    assert(res.instructions[1].mnemonic == "POP" && res.instructions[1].operands[0].reg == 1);
}

void test_arithmetic() {
    const auto res = NeoParser::parse(
        "ADD r0, r1, r2\n"
        "SUB r3, r4, r5\n"
        "MUL r6, r7, r8\n"
        "DIV r9, r10, r11\n"
        "MOD r12, r13, r14\n"
        "NEG r15, r16\n"
        "ADD32 r17, r18, r19\n"
        "SUB32 r20, r21, r22\n"
    );
    assert(res.success && res.instructions.size() == 8);
    assert(res.instructions[0].mnemonic == "ADD" && res.instructions[0].operands.size() == 3);
    assert(res.instructions[1].mnemonic == "SUB");
    assert(res.instructions[2].mnemonic == "MUL");
    assert(res.instructions[3].mnemonic == "DIV");
    assert(res.instructions[4].mnemonic == "MOD");
    assert(res.instructions[5].mnemonic == "NEG" && res.instructions[5].operands.size() == 2);
    assert(res.instructions[6].mnemonic == "ADD32");
    assert(res.instructions[7].mnemonic == "SUB32");
}

void test_bitwise() {
    const auto res = NeoParser::parse(
        "AND r0, r1, r2\n"
        "OR r3, r4, r5\n"
        "XOR r6, r7, r8\n"
        "NOT r9, r10\n"
        "SHL r11, r12, r13\n"
        "SHR r14, r15, r16\n"
        "SAR r17, r18, r19\n"
        "CLZ r20, r21\n"
    );
    assert(res.success && res.instructions.size() == 8);
    assert(res.instructions[0].mnemonic == "AND");
    assert(res.instructions[3].mnemonic == "NOT" && res.instructions[3].operands.size() == 2);
    assert(res.instructions[7].mnemonic == "CLZ" && res.instructions[7].operands.size() == 2);
}

void test_branches_and_subroutines() {
    const auto res = NeoParser::parse(
        "JMP target\n"
        "JEQ r0, r1, target\n"
        "JNE r2, r3, target\n"
        "JGT r4, r5, target\n"
        "JGE r6, r7, target\n"
        "JLT r8, r9, target\n"
        "JLE r10, r11, target\n"
        "CALL func\n"
        "RET\n"
        "SYS 1\n"
        "BRK\n"
        "NOP\n"
        "FENCE\n"
    );
    assert(res.success && res.instructions.size() == 13);
    assert(res.instructions[0].mnemonic == "JMP" && res.instructions[0].operands[0].label == "target");
    assert(res.instructions[1].mnemonic == "JEQ" && res.instructions[1].operands.size() == 3);
    assert(res.instructions[4].mnemonic == "JGE" && res.instructions[4].operands[2].label == "target");
    assert(res.instructions[7].mnemonic == "CALL" && res.instructions[7].operands[0].label == "func");
    assert(res.instructions[8].mnemonic == "RET" && res.instructions[8].operands.empty());
    assert(res.instructions[9].mnemonic == "SYS" && res.instructions[9].operands[0].immediate == 1);
    assert(res.instructions[10].mnemonic == "BRK" && res.instructions[10].operands.empty());
    assert(res.instructions[11].mnemonic == "NOP" && res.instructions[11].operands.empty());
    assert(res.instructions[12].mnemonic == "FENCE" && res.instructions[12].operands.empty());
}

void test_origin_passthrough() {
    const auto res = NeoParser::parse(
        "origin(SYSCALL)\n"
        "origin(SVC 0)\n"
        "origin(MOV) r0, r1\n"
    );
    assert(res.success && res.instructions.size() == 3);
    assert(res.instructions[0].is_origin && res.instructions[0].origin_command == "SYSCALL");
    assert(res.instructions[1].is_origin && res.instructions[1].origin_command == "SVC 0");
    assert(res.instructions[2].is_origin && res.instructions[2].origin_command == "MOV");
    assert(res.instructions[2].operands.size() == 2);
}

} // namespace

void run_instruction_tests() {
    test_register_assign();
    test_memory_load_store();
    test_stack_operations();
    test_arithmetic();
    test_bitwise();
    test_branches_and_subroutines();
    test_origin_passthrough();
    std::cout << "[PASS] Instruction parser test suite\n";
}

} // namespace neo::test
