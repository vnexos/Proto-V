/**
 * @file template_builder.hpp
 * @brief Constructs packed NeoInstructionTemplate records from YAML specifications.
 */

#pragma once

#include "yaml_model.hpp"

#include <neo/schema.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace neo {

/**
 * @brief Container holding packed template records and section byte offsets.
 */
struct TemplateBuildResult {
    std::vector<NeoInstructionTemplate> templates;
    std::unordered_map<std::string, uint64_t> origin_offsets;
    std::unordered_map<std::string, uint64_t> neo_offsets;
    std::unordered_map<std::string, uint16_t> neo_counts;
};

/**
 * @brief Packs all native and unified instruction templates into a contiguous buffer.
 *
 * @param spec Parsed architecture specification.
 * @return TemplateBuildResult containing template records and offset mappings.
 */
TemplateBuildResult build_instruction_templates(const ParsedArchitectureSpec& spec);

} // namespace neo
