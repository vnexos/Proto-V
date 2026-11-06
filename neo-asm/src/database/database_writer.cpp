/**
 * @file database_writer.cpp
 * @brief Serializes architecture specifications into 8-byte aligned .neodb binary files.
 */

#include "database_writer.hpp"
#include "template_builder.hpp"

#include <neo/common.hpp>
#include <neo/schema.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <unordered_set>

namespace neo {

namespace {

struct StringTableData {
    std::vector<uint8_t> bytes;
    std::unordered_map<std::string, uint32_t> offsets;
};

StringTableData build_string_table(const ParsedArchitectureSpec& spec) {
    StringTableData st;
    st.bytes.push_back('\0'); // Offset 0 is empty string

    auto intern_string = [&](const std::string& str) -> uint32_t {
        const auto it = st.offsets.find(str);
        if (it != st.offsets.end()) return it->second;

        const uint32_t off = static_cast<uint32_t>(st.bytes.size());
        st.offsets[str] = off;
        st.bytes.insert(st.bytes.end(), str.begin(), str.end());
        st.bytes.push_back('\0');
        return off;
    };

    for (const auto& op : spec.neo_ops) intern_string(op.name);
    for (const auto& op : spec.origin_ops) {
        intern_string("origin(" + op.name + ")");
        intern_string(op.name);
    }

    intern_string("__neo_meta__");
    if (!spec.raw_yaml_metadata.empty()) intern_string(spec.raw_yaml_metadata);

    return st;
}

std::vector<NeoIndexEntry> create_index_entries(
    const ParsedArchitectureSpec& spec,
    const TemplateBuildResult& tmpl_res,
    const std::unordered_map<std::string, uint32_t>& str_offsets)
{
    std::vector<NeoIndexEntry> entries;
    std::unordered_set<std::string> neo_names;

    for (const auto& op : spec.neo_ops) {
        neo_names.insert(op.name);
        const uint16_t count = tmpl_res.neo_counts.at(op.name);
        const uint64_t off = tmpl_res.neo_offsets.at(op.name);
        entries.push_back({str_offsets.at(op.name), fnv1a_32(op.name),
                           count > 1 ? 2u : 0u, count, 0, off});
    }

    for (const auto& op : spec.origin_ops) {
        const uint64_t off = tmpl_res.origin_offsets.at(op.name);
        const std::string origin_key = "origin(" + op.name + ")";
        entries.push_back({str_offsets.at(origin_key), fnv1a_32(origin_key), 1, 1, 0, off});

        // Bare mnemonic query supported if not shadowed by unified Neo-ASM opcode
        if (!neo_names.contains(op.name)) {
            entries.push_back({str_offsets.at(op.name), fnv1a_32(op.name), 1, 1, 0, off});
        }
    }

    if (!spec.raw_yaml_metadata.empty()) {
        const uint64_t meta_off = str_offsets.at(spec.raw_yaml_metadata);
        entries.push_back({str_offsets.at("__neo_meta__"), fnv1a_32("__neo_meta__"),
                           0x80000000u, 0, 0, meta_off});
    }
    return entries;
}

bool populate_frozen_index(std::vector<NeoIndexEntry>& entries,
                           std::vector<NeoIndexEntry>& buckets,
                           const StringTableData& st) noexcept
{
    // Deterministic sorting guarantees identical bucket placement across roundtrips
    std::sort(entries.begin(), entries.end(), [&](const auto& a, const auto& b) {
        return std::string_view(reinterpret_cast<const char*>(st.bytes.data() + a.name_str_offset)) <
               std::string_view(reinterpret_cast<const char*>(st.bytes.data() + b.name_str_offset));
    });

    for (const auto& entry : entries) {
        uint32_t idx = entry.name_hash % 256;
        bool placed = false;
        for (uint32_t probe = 0; probe < 256; ++probe) {
            if (buckets[idx].name_hash == 0) {
                buckets[idx] = entry;
                placed = true;
                break;
            }
            idx = (idx + 1) % 256;
        }
        if (!placed) return false;
    }
    return true;
}

} // namespace

bool write_neodb_file(const ParsedArchitectureSpec& spec, const std::filesystem::path& output_path) {
    const auto tmpl_res = build_instruction_templates(spec);
    const auto st = build_string_table(spec);
    auto entries = create_index_entries(spec, tmpl_res, st.offsets);

    std::vector<NeoIndexEntry> buckets(256);
    std::memset(buckets.data(), 0, buckets.size() * sizeof(NeoIndexEntry));
    if (!populate_frozen_index(entries, buckets, st)) return false;

    // Strict 8-byte alignment for all sections
    const uint64_t strtab_off = 64;
    const uint32_t strtab_size = static_cast<uint32_t>(st.bytes.size());
    const size_t strtab_pad = (8 - (strtab_size % 8)) % 8;

    const uint64_t idx_off = strtab_off + strtab_size + strtab_pad;
    const uint32_t idx_size = static_cast<uint32_t>(buckets.size() * sizeof(NeoIndexEntry));
    const size_t idx_pad = (8 - (idx_size % 8)) % 8;

    const uint64_t tmpl_off = idx_off + idx_size + idx_pad;
    const uint32_t tmpl_size = static_cast<uint32_t>(tmpl_res.templates.size() * sizeof(NeoInstructionTemplate));
    const size_t tmpl_pad = (8 - (tmpl_size % 8)) % 8;

    const size_t total_size = tmpl_off + tmpl_size + tmpl_pad;
    std::vector<uint8_t> buffer(total_size, 0);

    auto* hdr = reinterpret_cast<NeoDbHeader*>(buffer.data());
    std::memcpy(hdr->magic, "NEODB\0\0\0", 8);
    hdr->format_version = 0x00010000;
    hdr->arch_id = spec.arch_id;
    hdr->arch_flags = (spec.endian == "big" ? USX_AFLAG_BIG_ENDIAN : 0) |
                      (spec.native_registers >= 32 ? USX_AFLAG_REG_COUNT : 0);
    hdr->strtab_offset = strtab_off;
    hdr->strtab_size = strtab_size;
    hdr->strtab_count = static_cast<uint32_t>(st.offsets.size());
    hdr->index_offset = idx_off;
    hdr->index_bucket_count = 256;
    hdr->index_entry_count = static_cast<uint32_t>(entries.size());
    hdr->template_offset = tmpl_off;
    hdr->template_size = tmpl_size;

    std::memcpy(buffer.data() + strtab_off, st.bytes.data(), st.bytes.size());
    std::memcpy(buffer.data() + idx_off, buckets.data(), idx_size);
    std::memcpy(buffer.data() + tmpl_off, tmpl_res.templates.data(), tmpl_size);

    hdr->header_crc32 = calcCRC32(std::span<const uint8_t>(buffer.data(), 60));

    std::ofstream out(output_path, std::ios::binary);
    if (!out.is_open()) return false;
    out.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    return out.good();
}

} // namespace neo
