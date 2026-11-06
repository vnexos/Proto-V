/**
 * @file yaml_model.hpp
 * @brief Intermediate representations for parsed YAML architecture definitions.
 */

#pragma once

#include <neo/schema.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace neo {

/**
 * @brief Parsed operand slot configuration extracted from YAML origin op.
 */
struct ParsedOperandSlot {
    uint8_t slot{0};
    NeoOperandType type{OPERAND_NONE};
    uint8_t bit_offset{0};
    uint8_t bit_width{0};
    uint8_t flags{0};
    uint32_t mask{0};
};

/**
 * @brief Parsed native CPU instruction from ops.origin section.
 */
struct ParsedOriginOp {
    std::string name;
    std::string description;
    std::string opcode_hex;
    uint8_t byte_length{0};
    std::vector<ParsedOperandSlot> operands;
};

/**
 * @brief Parsed unified Neo-ASM instruction from ops.neo section.
 */
struct ParsedNeoOp {
    std::string name;
    std::string description;
    std::vector<std::string> operands;
    std::vector<std::pair<std::string, std::vector<std::string>>> expansions;
};

/**
 * @brief Parsed architecture definition containing metadata and instruction sets.
 */
struct ParsedArchitectureSpec {
    std::string arch_name;
    uint16_t arch_id{0};
    std::string endian{"little"};
    uint32_t registers{32};
    uint16_t native_registers{16};
    bool pic_mode{true};
    std::string raw_yaml_metadata;
    std::vector<ParsedOriginOp> origin_ops;
    std::vector<ParsedNeoOp> neo_ops;
};

} // namespace neo
