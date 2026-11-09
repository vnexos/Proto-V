/**
 * @file compiler_instruction_planner.cpp
 * @brief Implementation of template resolution and concrete instruction planning.
 */

#include "compiler_instruction_planner.hpp"

namespace neo::detail {

namespace {

const NeoIndexEntry* resolve_entry(
    const NeoDatabase& db,
    const ExpandedInstruction& inst,
    uint16_t arch_id) {
    if (arch_id == USX_ARCH_X86_64 && inst.mnemonic == "MOV" && inst.operands.size() >= 2) {
        if (inst.operands[0].kind == ExpandedOperandKind::Register && inst.operands[1].kind == ExpandedOperandKind::Memory) {
            return db.find_entry("MOV_LOAD");
        }
        if (inst.operands[0].kind == ExpandedOperandKind::Memory && inst.operands[1].kind == ExpandedOperandKind::Register) {
            return db.find_entry("MOV_STORE");
        }
    }

    if (inst.mnemonic == "ASN" && inst.operands.size() >= 2) {
        const bool load = (inst.operands[0].kind == ExpandedOperandKind::Register && inst.operands[1].kind == ExpandedOperandKind::Memory);
        const bool store = (inst.operands[0].kind == ExpandedOperandKind::Memory && inst.operands[1].kind == ExpandedOperandKind::Register);
        if (load || store) {
            if (arch_id == USX_ARCH_X86_64) return db.find_entry(load ? "MOV_LOAD" : "MOV_STORE");
            if (arch_id == USX_ARCH_AARCH64) return db.find_entry(load ? "LDR" : "STR");
            if (arch_id == USX_ARCH_RISCV64) return db.find_entry(load ? "LD" : "SD");
        }
    }

    if (const auto* e = db.find_entry(inst.mnemonic)) return e;

    if (inst.mnemonic.starts_with("origin(") && inst.mnemonic.ends_with(")")) {
        const auto inner = inst.mnemonic.substr(7, inst.mnemonic.size() - 8);
        const auto space = inner.find(' ');
        const auto cmd = (space == std::string::npos) ? inner : inner.substr(0, space);
        if (const auto* e = db.find_entry("origin(" + std::string(cmd) + ")")) return e;
        if (const auto* e = db.find_entry(cmd)) return e;
    }

    return nullptr;
}

int64_t extract_slot_val(
    const NeoOperandSlot& slot,
    size_t slot_idx,
    const ExpandedInstruction& inst,
    uint16_t arch_id) {
    if (slot.slot_type == OPERAND_REG_GPR) {
        if (arch_id == USX_ARCH_AARCH64 && inst.mnemonic == "ASN" && inst.operands.size() == 2) {
            if (slot_idx == 0) return inst.operands[0].reg;
            if (slot_idx == 1) return 31; // XZR
            return inst.operands[1].reg;
        }
        if (arch_id == USX_ARCH_X86_64 && inst.operands.size() >= 2) {
            if (inst.operands[0].kind == ExpandedOperandKind::Register && inst.operands[1].kind == ExpandedOperandKind::Memory) {
                return (slot_idx == 0) ? inst.operands[1].mem.base_reg : inst.operands[0].reg;
            }
            if (inst.operands[0].kind == ExpandedOperandKind::Memory && inst.operands[1].kind == ExpandedOperandKind::Register) {
                return (slot_idx == 0) ? inst.operands[0].mem.base_reg : inst.operands[1].reg;
            }
        }
        if (slot_idx < inst.operands.size()) {
            return (inst.operands[slot_idx].kind == ExpandedOperandKind::Memory)
                ? inst.operands[slot_idx].mem.base_reg
                : inst.operands[slot_idx].reg;
        }
    }
    if (slot.slot_type == OPERAND_MEM_DISP) {
        for (const auto& op : inst.operands) {
            if (op.kind == ExpandedOperandKind::Memory) return op.mem.offset;
        }
    }
    if (slot.slot_type == OPERAND_IMM_INT) {
        for (const auto& op : inst.operands) {
            if (op.kind == ExpandedOperandKind::Immediate) return op.immediate;
        }
    }
    return 0;
}

ConcreteInstruction build_concrete_insn(
    const NeoInstructionTemplate& tmpl,
    const ExpandedInstruction& inst,
    const std::string& scope,
    size_t ast_idx,
    uint16_t arch_id) {
    ConcreteInstruction ci{};
    ci.tmpl = tmpl;
    ci.ast_index = ast_idx;
    ci.location = inst.location;
    ci.branch_scope = scope;

    // x86_64 register-direct ModR/M requires Mod=11 (0xC0) when both operands are GPRs
    if (arch_id == USX_ARCH_X86_64 && tmpl.byte_length >= 3 && tmpl.operand_count >= 2) {
        if (tmpl.slots[0].slot_type == OPERAND_REG_GPR && tmpl.slots[1].slot_type == OPERAND_REG_GPR) {
            if ((ci.tmpl.opcode_bytes[2] & 0xC0) == 0) {
                ci.tmpl.opcode_bytes[2] |= 0xC0;
            }
        }
    }

    // Convert big-endian word hex from YAML to little-endian word layout expected by emitter
    if ((arch_id == USX_ARCH_AARCH64 || arch_id == USX_ARCH_RISCV64) &&
        tmpl.byte_length == 4 && tmpl.operand_count > 0) {
        std::swap(ci.tmpl.opcode_bytes[0], ci.tmpl.opcode_bytes[3]);
        std::swap(ci.tmpl.opcode_bytes[1], ci.tmpl.opcode_bytes[2]);
    }

    for (uint8_t s = 0; s < tmpl.operand_count; ++s) {
        const auto& slot = tmpl.slots[s];
        if (slot.slot_type == OPERAND_PIC_REL_TAG) {
            ci.has_pic_branch = true;
            ci.pic_slot_index = s;
            for (const auto& op : inst.operands) {
                if (op.kind == ExpandedOperandKind::Label) ci.branch_target_label = op.label;
            }
            ci.slot_values.push_back(0);
        } else {
            ci.slot_values.push_back(extract_slot_val(slot, s, inst, arch_id));
        }
    }
    return ci;
}

} // namespace

bool plan_instruction(
    const NeoDatabase& db,
    const ExpandedInstruction& inst,
    const std::string& scope,
    size_t ast_idx,
    uint16_t arch_id,
    std::vector<ConcreteInstruction>& out,
    std::vector<std::string>& errors) {
    const auto* entry = resolve_entry(db, inst, arch_id);
    if (!entry) {
        errors.push_back("Unknown instruction mnemonic: " + inst.mnemonic);
        return false;
    }
    const auto tmpls = db.get_templates(*entry);
    if (tmpls.empty()) {
        errors.push_back("No instruction templates found for: " + inst.mnemonic);
        return false;
    }
    if (tmpls.size() >= 2 && inst.operands.size() >= 3) {
        out.push_back(build_concrete_insn(tmpls[0], {inst.mnemonic, {inst.operands[0], inst.operands[1]}, false, inst.location}, scope, ast_idx, arch_id));
        out.push_back(build_concrete_insn(tmpls[1], {inst.mnemonic, {inst.operands[0], inst.operands[2]}, false, inst.location}, scope, ast_idx, arch_id));
        return true;
    }
    const auto& selected = (tmpls.size() >= 2 && inst.operands.size() == 2) ? tmpls[1] : tmpls[0];
    out.push_back(build_concrete_insn(selected, inst, scope, ast_idx, arch_id));
    return true;
}

} // namespace neo::detail
