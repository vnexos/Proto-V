/**
 * @file x86_spill_sequencer.cpp
 * @brief Synthesis of scratch load/store sequences for x86_64 spilled virtual registers.
 */

#include "x86_spill_sequencer.hpp"
#include "register_mapping.hpp"

namespace neo::detail {

namespace {

ExpandedOperand scratch_rax() {
    return {ExpandedOperandKind::Register, 0, "RAX", 0, {}, ""};
}

ExpandedOperand scratch_rdx() {
    return {ExpandedOperandKind::Register, 2, "RDX", 0, {}, ""};
}

ExpandedOperand spill_slot(int32_t offset) {
    ExpandedOperand op{ExpandedOperandKind::Memory, REG_INVALID, "", 0, {}, ""};
    op.mem.base_reg = 5; // RBP
    op.mem.offset = -offset;
    op.mem.base_name = "rbp";
    return op;
}

ExpandedOperand to_expanded_operand(const Operand& op) {
    if (op.kind == OperandKind::Register) {
        const auto map = map_x86_64(op.reg, op.is_system_reg);
        if (map.is_spilled) {
            return spill_slot(map.spill_offset);
        }
        return {ExpandedOperandKind::Register, map.hardware_id, map.name, 0, {}, ""};
    }
    if (op.kind == OperandKind::Immediate) {
        return {ExpandedOperandKind::Immediate, REG_INVALID, "", op.immediate, {}, ""};
    }
    if (op.kind == OperandKind::Memory) {
        ExpandedOperand exp{ExpandedOperandKind::Memory, REG_INVALID, "", 0, {}, ""};
        const auto base_map = map_x86_64(op.mem.base_reg, op.mem.is_base_system);
        exp.mem.base_reg = base_map.hardware_id;
        exp.mem.offset = op.mem.displacement;
        exp.mem.base_name = base_map.name;
        return exp;
    }
    return {ExpandedOperandKind::Label, REG_INVALID, "", 0, {}, op.label};
}

void expand_asn(const AstInstruction& ast, std::vector<ExpandedInstruction>& out) {
    const auto& dst = ast.operands[0];
    const auto& src = ast.operands[1];
    const bool dst_spilled = (dst.kind == OperandKind::Register && NeoRegisterAllocator::is_spilled(dst.reg, USX_ARCH_X86_64));
    const bool src_spilled = (src.kind == OperandKind::Register && NeoRegisterAllocator::is_spilled(src.reg, USX_ARCH_X86_64));

    if (dst_spilled && src_spilled) {
        // Dual spill: copy through scratch RAX
        out.push_back({"MOV", {scratch_rax(), spill_slot(NeoRegisterAllocator::spill_offset(src.reg))}, true, ast.location});
        out.push_back({"MOV", {spill_slot(NeoRegisterAllocator::spill_offset(dst.reg)), scratch_rax()}, true, ast.location});
        return;
    }
    if (dst_spilled) {
        out.push_back({"MOV", {spill_slot(NeoRegisterAllocator::spill_offset(dst.reg)), to_expanded_operand(src)}, true, ast.location});
        return;
    }
    if (src_spilled) {
        out.push_back({"MOV", {to_expanded_operand(dst), spill_slot(NeoRegisterAllocator::spill_offset(src.reg))}, true, ast.location});
        return;
    }
    out.push_back({"MOV", {to_expanded_operand(dst), to_expanded_operand(src)}, false, ast.location});
}

void expand_stack(const AstInstruction& ast, std::vector<ExpandedInstruction>& out) {
    const auto& reg_op = ast.operands[0];
    const bool spilled = (reg_op.kind == OperandKind::Register && NeoRegisterAllocator::is_spilled(reg_op.reg, USX_ARCH_X86_64));

    if (ast.mnemonic == "PUSH") {
        if (spilled) {
            out.push_back({"MOV", {scratch_rax(), spill_slot(NeoRegisterAllocator::spill_offset(reg_op.reg))}, true, ast.location});
            out.push_back({"PUSH", {scratch_rax()}, true, ast.location});
        } else {
            out.push_back({"PUSH", {to_expanded_operand(reg_op)}, false, ast.location});
        }
        return;
    }
    if (spilled) {
        out.push_back({"POP", {scratch_rax()}, true, ast.location});
        out.push_back({"MOV", {spill_slot(NeoRegisterAllocator::spill_offset(reg_op.reg)), scratch_rax()}, true, ast.location});
    } else {
        out.push_back({"POP", {to_expanded_operand(reg_op)}, false, ast.location});
    }
}

void expand_arith_dual_spill(const AstInstruction& ast, std::vector<ExpandedInstruction>& out,
                             uint8_t dst_reg, uint8_t s1_reg, uint8_t s2_reg) {
    // Utilize RAX for src1 and RDX for src2 to avoid clobbering
    out.push_back({"MOV", {scratch_rax(), spill_slot(NeoRegisterAllocator::spill_offset(s1_reg))}, true, ast.location});
    out.push_back({"MOV", {scratch_rdx(), spill_slot(NeoRegisterAllocator::spill_offset(s2_reg))}, true, ast.location});
    out.push_back({ast.mnemonic, {scratch_rax(), scratch_rdx()}, true, ast.location});
    out.push_back({"MOV", {spill_slot(NeoRegisterAllocator::spill_offset(dst_reg)), scratch_rax()}, true, ast.location});
}

void expand_arith_single_spill(const AstInstruction& ast, std::vector<ExpandedInstruction>& out,
                               uint8_t dst_reg, const Operand& s2_op) {
    // Load destination spill slot into scratch RAX, execute op with source, and store back
    out.push_back({"MOV", {scratch_rax(), spill_slot(NeoRegisterAllocator::spill_offset(dst_reg))}, true, ast.location});
    out.push_back({ast.mnemonic, {scratch_rax(), to_expanded_operand(s2_op)}, true, ast.location});
    out.push_back({"MOV", {spill_slot(NeoRegisterAllocator::spill_offset(dst_reg)), scratch_rax()}, true, ast.location});
}

void expand_arithmetic(const AstInstruction& ast, std::vector<ExpandedInstruction>& out) {
    const auto& dst = ast.operands[0];
    const auto& s1 = (ast.operands.size() >= 3) ? ast.operands[1] : ast.operands[0];
    const auto& s2 = (ast.operands.size() >= 3) ? ast.operands[2] : ast.operands[1];

    const bool dst_sp = (dst.kind == OperandKind::Register && NeoRegisterAllocator::is_spilled(dst.reg, USX_ARCH_X86_64));
    const bool s1_sp = (s1.kind == OperandKind::Register && NeoRegisterAllocator::is_spilled(s1.reg, USX_ARCH_X86_64));
    const bool s2_sp = (s2.kind == OperandKind::Register && NeoRegisterAllocator::is_spilled(s2.reg, USX_ARCH_X86_64));

    if (s1_sp && s2_sp) {
        expand_arith_dual_spill(ast, out, dst.reg, s1.reg, s2.reg);
        return;
    }
    if (dst_sp) {
        expand_arith_single_spill(ast, out, dst.reg, s2);
        return;
    }
    // No destination spill: emit native instruction directly
    std::vector<ExpandedOperand> exp_ops;
    for (const auto& op : ast.operands) {
        exp_ops.push_back(to_expanded_operand(op));
    }
    out.push_back({ast.mnemonic, std::move(exp_ops), false, ast.location});
}

} // namespace

bool has_spilled_operand(const AstInstruction& ast) {
    for (const auto& op : ast.operands) {
        if (op.kind == OperandKind::Register && NeoRegisterAllocator::is_spilled(op.reg, USX_ARCH_X86_64)) {
            return true;
        }
    }
    return false;
}

std::vector<ExpandedInstruction> expand_x86_instruction(const AstInstruction& ast) {
    if (ast.operands.empty()) {
        return {{ast.mnemonic, {}, false, ast.location}};
    }
    if (ast.is_origin) {
        std::vector<ExpandedOperand> exp_ops;
        exp_ops.reserve(ast.operands.size());
        for (const auto& op : ast.operands) {
            exp_ops.push_back(to_expanded_operand(op));
        }
        return {{ast.mnemonic, std::move(exp_ops), false, ast.location}};
    }
    std::vector<ExpandedInstruction> out;
    if (ast.mnemonic == "ASN") {
        expand_asn(ast, out);
        return out;
    }
    if (ast.mnemonic == "PUSH" || ast.mnemonic == "POP") {
        expand_stack(ast, out);
        return out;
    }
    expand_arithmetic(ast, out);
    return out;
}

} // namespace neo::detail
