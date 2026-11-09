/**
 * @file register_alloc.cpp
 * @brief Implementations of register allocator facade and instruction expansion pipeline.
 */

#include "neo/register_alloc.hpp"
#include "register_mapping.hpp"
#include "x86_spill_sequencer.hpp"

namespace neo {

std::string ExpandedInstruction::to_string() const {
    if (operands.empty()) {
        return mnemonic;
    }
    std::string result = mnemonic + " ";
    for (size_t i = 0; i < operands.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        const auto& op = operands[i];
        if (op.kind == ExpandedOperandKind::Register) {
            result += op.reg_name;
        } else if (op.kind == ExpandedOperandKind::Immediate) {
            result += std::to_string(op.immediate);
        } else if (op.kind == ExpandedOperandKind::Memory) {
            result += "[" + op.mem.base_name;
            if (op.mem.offset < 0) {
                result += " - " + std::to_string(-op.mem.offset);
            } else if (op.mem.offset > 0) {
                result += " + " + std::to_string(op.mem.offset);
            }
            result += "]";
        } else if (op.kind == ExpandedOperandKind::Label) {
            result += op.label;
        }
    }
    return result;
}

uint16_t NeoRegisterAllocator::resolve_arch_id(std::string_view arch) noexcept {
    if (arch == "x86_64" || arch == "X86_64" || arch == "x86-64") {
        return USX_ARCH_X86_64;
    }
    if (arch == "aarch64" || arch == "AARCH64" || arch == "arm64") {
        return USX_ARCH_AARCH64;
    }
    if (arch == "riscv64" || arch == "RISCV64" || arch == "riscv") {
        return USX_ARCH_RISCV64;
    }
    return 0;
}

bool NeoRegisterAllocator::is_spilled(uint8_t reg, uint16_t arch_id) noexcept {
    if (arch_id != USX_ARCH_X86_64) {
        return false;
    }
    return reg >= 14 && reg <= 31;
}

RegisterMapping NeoRegisterAllocator::map_register(uint8_t reg, bool is_system, uint16_t arch_id) {
    if (arch_id == USX_ARCH_AARCH64) {
        return detail::map_aarch64(reg, is_system);
    }
    if (arch_id == USX_ARCH_RISCV64) {
        return detail::map_riscv64(reg, is_system);
    }
    return detail::map_x86_64(reg, is_system);
}

namespace {

ExpandedOperand to_native_operand(const Operand& op, uint16_t arch_id) {
    if (op.kind == OperandKind::Register) {
        const auto map = NeoRegisterAllocator::map_register(op.reg, op.is_system_reg, arch_id);
        return {ExpandedOperandKind::Register, map.hardware_id, map.name, 0, {}, ""};
    }
    if (op.kind == OperandKind::Immediate) {
        return {ExpandedOperandKind::Immediate, REG_INVALID, "", op.immediate, {}, ""};
    }
    if (op.kind == OperandKind::Memory) {
        ExpandedOperand exp{ExpandedOperandKind::Memory, REG_INVALID, "", 0, {}, ""};
        const auto base_map = NeoRegisterAllocator::map_register(op.mem.base_reg, op.mem.is_base_system, arch_id);
        exp.mem.base_reg = base_map.hardware_id;
        exp.mem.offset = op.mem.displacement;
        exp.mem.base_name = base_map.name;
        return exp;
    }
    return {ExpandedOperandKind::Label, REG_INVALID, "", 0, {}, op.label};
}

} // namespace

std::vector<ExpandedInstruction> NeoRegisterAllocator::expand_instruction(
    const AstInstruction& ast, uint16_t arch_id) {
    if (arch_id == USX_ARCH_X86_64) {
        return detail::expand_x86_instruction(ast);
    }

    // AArch64 and RISC-V 64: 1:1 hardware register mapping without spill sequences
    std::vector<ExpandedOperand> exp_ops;
    exp_ops.reserve(ast.operands.size());
    for (const auto& op : ast.operands) {
        exp_ops.push_back(to_native_operand(op, arch_id));
    }
    return {{ast.mnemonic, std::move(exp_ops), false, ast.location}};
}

std::vector<ExpandedInstruction> NeoRegisterAllocator::expand_program(
    const std::vector<AstInstruction>& instructions, uint16_t arch_id) {
    std::vector<ExpandedInstruction> result;
    for (const auto& insn : instructions) {
        auto expanded = expand_instruction(insn, arch_id);
        result.insert(result.end(), expanded.begin(), expanded.end());
    }
    return result;
}

std::vector<ExpandedInstruction> NeoRegisterAllocator::expand_source(
    std::string_view source, uint16_t arch_id) {
    const auto parse_res = NeoParser::parse(source);
    if (!parse_res.success) {
        return {};
    }
    return expand_program(parse_res.instructions, arch_id);
}

} // namespace neo
