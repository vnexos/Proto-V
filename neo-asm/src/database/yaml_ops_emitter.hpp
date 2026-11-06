/**
 * @file yaml_ops_emitter.hpp
 * @brief Serializes decompiled Neo-ASM and native origin opcodes into YAML DOM.
 */

#pragma once

#include <neo/database.hpp>
#include <neo/schema.hpp>
#include <yaml-cpp/yaml.h>

#include <map>
#include <string>
#include <unordered_map>

namespace neo {

/**
 * @brief Emits unified Neo-ASM instructions with default origin expansion sequences.
 */
void emit_neo_section(
    YAML::Emitter& out,
    const NeoDatabase& db,
    const std::map<std::string, const NeoIndexEntry*>& neo_entries,
    const std::unordered_map<uint16_t, std::string>& id_to_origin,
    const std::map<std::string, const NeoIndexEntry*>& origin_entries);

/**
 * @brief Emits native architecture opcode definitions with opcode bytes and operand slots.
 */
void emit_origin_section(
    YAML::Emitter& out,
    const NeoDatabase& db,
    const std::map<std::string, const NeoIndexEntry*>& origin_entries);

} // namespace neo
