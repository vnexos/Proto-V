/**
 * @file yaml_reader.cpp
 * @brief Parses YAML architecture definitions using yaml-cpp DOM.
 */

#include "yaml_reader.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>

namespace neo {

namespace {

/**
 * @brief Parses an unsigned 32-bit integer supporting hex strings and decimal numbers.
 */
uint32_t parse_scalar_uint32(const YAML::Node& node, uint32_t default_val = 0) noexcept {
    if (!node.IsDefined() || node.IsNull()) return default_val;

    try {
        const std::string text = node.Scalar();
        if (text.empty()) return default_val;

        // Base 16 is required when strings contain hexadecimal notation
        if (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0) {
            return static_cast<uint32_t>(std::stoul(text, nullptr, 16));
        }
        return static_cast<uint32_t>(std::stoul(text, nullptr, 10));
    } catch (...) {
        return default_val;
    }
}

/**
 * @brief Maps symbolic operand type names to canonical NeoOperandType enum values.
 */
NeoOperandType parse_slot_type(std::string_view type_str) noexcept {
    if (type_str == "REG_GPR")     return OPERAND_REG_GPR;
    if (type_str == "IMM_INT")     return OPERAND_IMM_INT;
    if (type_str == "MEM_DISP")    return OPERAND_MEM_DISP;
    if (type_str == "PIC_REL_TAG") return OPERAND_PIC_REL_TAG;
    if (type_str == "SPILL_SLOT")  return OPERAND_SPILL_SLOT;
    return OPERAND_NONE;
}

ParsedOperandSlot parse_single_slot(const YAML::Node& item) {
    ParsedOperandSlot slot{};
    slot.slot = static_cast<uint8_t>(parse_scalar_uint32(item["slot"]));
    slot.type = parse_slot_type(item["type"] ? item["type"].Scalar() : "");
    slot.bit_offset = static_cast<uint8_t>(parse_scalar_uint32(item["bit_offset"]));
    slot.bit_width = static_cast<uint8_t>(parse_scalar_uint32(item["bit_width"]));
    slot.flags = static_cast<uint8_t>(parse_scalar_uint32(item["flags"]));
    slot.mask = parse_scalar_uint32(item["mask"]);
    return slot;
}

std::vector<ParsedOriginOp> parse_origin_section(const YAML::Node& origin_node) {
    std::vector<ParsedOriginOp> ops;
    if (!origin_node.IsMap()) return ops;

    for (auto it = origin_node.begin(); it != origin_node.end(); ++it) {
        ParsedOriginOp op{};
        op.name = it->first.Scalar();
        const auto& val = it->second;

        op.description = val["description"] ? val["description"].Scalar() : "";
        op.opcode_hex = val["opcode_hex"] ? val["opcode_hex"].Scalar() : "";
        op.byte_length = static_cast<uint8_t>(parse_scalar_uint32(val["byte_length"]));

        if (val["operands"] && val["operands"].IsSequence()) {
            for (const auto& item : val["operands"]) {
                op.operands.push_back(parse_single_slot(item));
            }
        }
        ops.push_back(std::move(op));
    }
    return ops;
}

std::vector<ParsedNeoOp> parse_neo_section(const YAML::Node& neo_node) {
    std::vector<ParsedNeoOp> ops;
    if (!neo_node.IsMap()) return ops;

    for (auto it = neo_node.begin(); it != neo_node.end(); ++it) {
        ParsedNeoOp op{};
        op.name = it->first.Scalar();
        const YAML::Node val = it->second;

        op.description = val["description"] ? val["description"].Scalar() : "";
        if (val["operands"] && val["operands"].IsSequence()) {
            for (const auto& item : val["operands"]) op.operands.push_back(item.Scalar());
        }

        const YAML::Node exp_node = val["expansion"];
        if (exp_node && exp_node.IsMap()) {
            for (auto exp_it = exp_node.begin(); exp_it != exp_node.end(); ++exp_it) {
                std::vector<std::string> sequence;
                const YAML::Node item_node = exp_it->second;
                if (item_node.IsSequence()) {
                    for (const auto& line : item_node) sequence.push_back(line.Scalar());
                }
                op.expansions.emplace_back(exp_it->first.Scalar(), std::move(sequence));
            }
        }
        ops.push_back(std::move(op));
    }
    return ops;
}

} // namespace

std::optional<ParsedArchitectureSpec> load_yaml_architecture(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;

    std::ostringstream sstr;
    sstr << file.rdbuf();
    const std::string raw_text = sstr.str();
    if (raw_text.empty()) return std::nullopt;

    try {
        const YAML::Node root = YAML::Load(raw_text);
        if (!root.IsMap()) return std::nullopt;

        ParsedArchitectureSpec spec{};
        spec.raw_yaml_metadata = raw_text;
        spec.arch_name = root["arch"] ? root["arch"].Scalar() : "";
        spec.arch_id = static_cast<uint16_t>(parse_scalar_uint32(root["arch_id"]));
        spec.endian = root["endian"] ? root["endian"].Scalar() : "little";
        spec.registers = parse_scalar_uint32(root["registers"], 32);
        spec.native_registers = static_cast<uint16_t>(parse_scalar_uint32(root["native_registers"], 16));
        spec.pic_mode = root["pic_mode"] ? root["pic_mode"].as<bool>(true) : true;

        if (root["ops"] && root["ops"].IsMap()) {
            spec.origin_ops = parse_origin_section(root["ops"]["origin"]);
            spec.neo_ops = parse_neo_section(root["ops"]["neo"]);
        }
        return spec;
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace neo
