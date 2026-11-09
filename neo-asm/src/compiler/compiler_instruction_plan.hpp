/**
 * @file compiler_instruction_plan.hpp
 * @brief Representation of planned concrete instructions and operand slot assignments.
 */

#pragma once

#include "neo/common.hpp"
#include "neo/parser.hpp"
#include "neo/schema.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace neo::detail {

/**
 * @brief A planned concrete machine instruction with assigned operand slot values.
 */
struct ConcreteInstruction {
    NeoInstructionTemplate tmpl{};
    std::vector<int64_t> slot_values;
    bool has_pic_branch{false};
    uint8_t pic_slot_index{0};
    std::string branch_target_label;
    std::string branch_scope;
    size_t ast_index{0};
    SourceLocation location{};
};

} // namespace neo::detail
