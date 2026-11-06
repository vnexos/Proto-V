/**
 * @file template_builder.cpp
 * @brief Implementation of template packing and offset resolution for .neodb databases.
 */

#include "template_builder.hpp"

#include <algorithm>
#include <cstring>

namespace neo {

namespace {

uint8_t parse_hex_nibble(char c) noexcept {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    return 0;
}

void parse_hex_bytes(std::string_view hex_str, uint8_t* out, size_t max_len) noexcept {
    const size_t byte_count = std::min(hex_str.size() / 2, max_len);
    for (size_t i = 0; i < byte_count; ++i) {
        const uint8_t hi = parse_hex_nibble(hex_str[2 * i]);
        const uint8_t lo = parse_hex_nibble(hex_str[2 * i + 1]);
        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
}

NeoInstructionTemplate make_origin_template(const ParsedOriginOp& op, uint16_t id) noexcept {
    NeoInstructionTemplate tmpl{};
    tmpl.instruction_id = id;
    tmpl.byte_length = op.byte_length;
    tmpl.operand_count = static_cast<uint8_t>(std::min<size_t>(op.operands.size(), 4));

    parse_hex_bytes(op.opcode_hex, tmpl.opcode_bytes, sizeof(tmpl.opcode_bytes));

    for (const auto& slot : op.operands) {
        if (slot.slot >= 4) continue;
        auto& dst = tmpl.slots[slot.slot];
        dst.slot_type = slot.type;
        dst.bit_offset = slot.bit_offset;
        dst.bit_width = slot.bit_width;
        dst.flags = slot.flags;
        dst.mask = slot.mask;
    }
    return tmpl;
}

std::string extract_origin_name(std::string_view line) {
    const auto start = line.find("origin(");
    if (start == std::string_view::npos) return {};

    const auto open_paren = start + 7;
    const auto close_paren = line.find(')', open_paren);
    if (close_paren == std::string_view::npos) return {};

    const auto inner = line.substr(open_paren, close_paren - open_paren);
    const auto space_pos = inner.find(' ');
    return std::string(space_pos == std::string_view::npos ? inner : inner.substr(0, space_pos));
}

} // namespace

TemplateBuildResult build_instruction_templates(const ParsedArchitectureSpec& spec) {
    TemplateBuildResult result;
    uint16_t next_id = 1;

    std::unordered_map<std::string, NeoInstructionTemplate> origin_map;
    for (const auto& op : spec.origin_ops) {
        const auto tmpl = make_origin_template(op, next_id++);
        origin_map[op.name] = tmpl;

        // Origin templates are placed first in .tmpl
        result.origin_offsets[op.name] = result.templates.size() * sizeof(NeoInstructionTemplate);
        result.templates.push_back(tmpl);
    }

    for (const auto& op : spec.neo_ops) {
        const size_t start_idx = result.templates.size();
        result.neo_offsets[op.name] = start_idx * sizeof(NeoInstructionTemplate);

        // Prefer default expansion sequence; fall back to load_mem or first available
        const auto it = std::find_if(op.expansions.begin(), op.expansions.end(),
            [](const auto& pair) { return pair.first == "default" || pair.first == "load_mem"; });

        const auto& seq = (it != op.expansions.end()) ? it->second :
                          (!op.expansions.empty() ? op.expansions.front().second : std::vector<std::string>{});

        for (const auto& line : seq) {
            const std::string origin_name = extract_origin_name(line);
            const auto orig_it = origin_map.find(origin_name);
            if (orig_it != origin_map.end()) {
                result.templates.push_back(orig_it->second);
            }
        }

        result.neo_counts[op.name] = static_cast<uint16_t>(result.templates.size() - start_idx);
    }

    return result;
}

} // namespace neo
