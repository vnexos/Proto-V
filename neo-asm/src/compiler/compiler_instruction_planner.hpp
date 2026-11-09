/**
 * @file compiler_instruction_planner.hpp
 * @brief Matches instruction templates in database and constructs concrete instruction plans.
 */

#pragma once

#include "compiler_instruction_plan.hpp"
#include "neo/database.hpp"
#include "neo/register_alloc.hpp"

#include <string>
#include <vector>

namespace neo::detail {

/**
 * @brief Plans machine code template and operand slot values for an expanded instruction.
 *
 * @param db Active architecture database.
 * @param inst Mapped expanded instruction.
 * @param scope Hierarchical label scope path.
 * @param ast_idx Source AST instruction sequence index.
 * @param arch_id Target architecture ID.
 * @param out Output vector of concrete instruction plans.
 * @param errors Diagnostic error sink.
 * @return True on success, false on unknown mnemonic or missing templates.
 */
bool plan_instruction(
    const NeoDatabase& db,
    const ExpandedInstruction& inst,
    const std::string& scope,
    size_t ast_idx,
    uint16_t arch_id,
    std::vector<ConcreteInstruction>& out,
    std::vector<std::string>& errors);

} // namespace neo::detail
