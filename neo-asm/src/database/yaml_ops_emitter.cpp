/**
 * @file yaml_ops_emitter.cpp
 * @brief Emits unified Neo-ASM and native origin opcodes with canonical hex formatting.
 */

#include "yaml_ops_emitter.hpp"

#include <cstdio>
#include <cstring>

namespace neo {

namespace {

/**
 * @brief Maps numeric operand slot type enum to its canonical string representation.
 */
std::string resolve_operand_type_name(uint8_t slot_type) noexcept {
    switch (slot_type) {
        case OPERAND_REG_GPR:     return "REG_GPR";
        case OPERAND_IMM_INT:     return "IMM_INT";
        case OPERAND_MEM_DISP:    return "MEM_DISP";
        case OPERAND_PIC_REL_TAG: return "PIC_REL_TAG";
        case OPERAND_SPILL_SLOT:  return "SPILL_SLOT";
        default:                  return "NONE";
    }
}

/**
 * @brief Emits an individual operand slot map with canonical hexadecimal mask formatting.
 */
void emit_operand_slot(YAML::Emitter& out, uint8_t slot_idx, const NeoOperandSlot& slot) {
    // Exact 8-digit uppercase hex formatting guarantees lossless roundtrip parsing
    char mask_buf[16];
    std::snprintf(mask_buf, sizeof(mask_buf), "0x%08X", slot.mask);

    out << YAML::BeginMap;
    out << YAML::Key << "slot" << YAML::Value << static_cast<int>(slot_idx);
    out << YAML::Key << "type" << YAML::Value << resolve_operand_type_name(slot.slot_type);
    out << YAML::Key << "bit_offset" << YAML::Value << static_cast<int>(slot.bit_offset);
    out << YAML::Key << "bit_width" << YAML::Value << static_cast<int>(slot.bit_width);
    if (slot.flags != 0) {
        out << YAML::Key << "flags" << YAML::Value << static_cast<int>(slot.flags);
    }
    out << YAML::Key << "mask" << YAML::Value << std::string(mask_buf);
    out << YAML::EndMap;
}

std::string find_origin_name_for_template(
    const NeoInstructionTemplate& tmpl,
    const std::unordered_map<uint16_t, std::string>& id_to_origin,
    const std::map<std::string, const NeoIndexEntry*>& origin_entries,
    const NeoDatabase& db)
{
    const auto it = id_to_origin.find(tmpl.instruction_id);
    if (it != id_to_origin.end()) {
        return it->second;
    }

    // Secondary fallback: match by opcode bytes
    for (const auto& [cand_name, cand_entry] : origin_entries) {
        const auto cand_tmpls = db.get_templates(*cand_entry);
        if (!cand_tmpls.empty() &&
            cand_tmpls[0].byte_length == tmpl.byte_length &&
            std::memcmp(cand_tmpls[0].opcode_bytes, tmpl.opcode_bytes, tmpl.byte_length) == 0) {
            return cand_name;
        }
    }
    return {};
}

} // namespace

void emit_neo_section(
    YAML::Emitter& out,
    const NeoDatabase& db,
    const std::map<std::string, const NeoIndexEntry*>& neo_entries,
    const std::unordered_map<uint16_t, std::string>& id_to_origin,
    const std::map<std::string, const NeoIndexEntry*>& origin_entries)
{
    out << YAML::Key << "neo" << YAML::Value << YAML::BeginMap;

    for (const auto& [name, entry_ptr] : neo_entries) {
        out << YAML::Key << name << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "description" << YAML::Value << ("Unified Neo-ASM " + name + " instruction");
        out << YAML::Key << "expansion" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "default" << YAML::Value << YAML::BeginSeq;

        const auto tmpls = db.get_templates(*entry_ptr);
        for (const auto& tmpl : tmpls) {
            const std::string orig_name = find_origin_name_for_template(tmpl, id_to_origin, origin_entries, db);
            if (!orig_name.empty()) {
                out << ("origin(" + orig_name + ")");
            }
        }
        out << YAML::EndSeq;
        out << YAML::EndMap; // expansion
        out << YAML::EndMap; // name
    }
    out << YAML::EndMap; // neo
}

void emit_origin_section(
    YAML::Emitter& out,
    const NeoDatabase& db,
    const std::map<std::string, const NeoIndexEntry*>& origin_entries)
{
    out << YAML::Key << "origin" << YAML::Value << YAML::BeginMap;

    for (const auto& [name, entry_ptr] : origin_entries) {
        const auto tmpls = db.get_templates(*entry_ptr);
        if (tmpls.empty()) continue;
        const auto& tmpl = tmpls[0];

        out << YAML::Key << name << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "description" << YAML::Value << ("Native " + name + " instruction");

        std::string hex_str;
        hex_str.reserve(tmpl.byte_length * 2);
        char hex_buf[4];
        for (size_t b = 0; b < tmpl.byte_length; ++b) {
            std::snprintf(hex_buf, sizeof(hex_buf), "%02X", tmpl.opcode_bytes[b]);
            hex_str.append(hex_buf);
        }
        out << YAML::Key << "opcode_hex" << YAML::Value << hex_str;
        out << YAML::Key << "byte_length" << YAML::Value << static_cast<int>(tmpl.byte_length);

        out << YAML::Key << "operands" << YAML::Value << YAML::BeginSeq;
        for (uint8_t s = 0; s < tmpl.operand_count; ++s) {
            emit_operand_slot(out, s, tmpl.slots[s]);
        }
        out << YAML::EndSeq; // operands
        out << YAML::EndMap; // name
    }
    out << YAML::EndMap; // origin
}

} // namespace neo
