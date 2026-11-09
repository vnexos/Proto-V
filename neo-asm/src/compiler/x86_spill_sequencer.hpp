/**
 * @file x86_spill_sequencer.hpp
 * @brief x86_64 stack spill and scratch sequence generator.
 */

#pragma once

#include "neo/register_alloc.hpp"

namespace neo::detail {

/**
 * @brief Checks if any register operand in the AST instruction is spilled to stack.
 */
[[nodiscard]] bool has_spilled_operand(const AstInstruction& ast);

/**
 * @brief Emits expanded instruction sequence for an AST instruction on x86_64.
 */
[[nodiscard]] std::vector<ExpandedInstruction> expand_x86_instruction(const AstInstruction& ast);

} // namespace neo::detail
