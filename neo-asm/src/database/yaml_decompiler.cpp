/**
 * @file yaml_decompiler.cpp
 * @brief Implementation of .neodb to formatted YAML decompiler coordinator.
 */

#include "yaml_decompiler.hpp"
#include "yaml_ops_emitter.hpp"

#include <neo/common.hpp>
#include <neo/database.hpp>
#include <neo/schema.hpp>

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo {

namespace {

/**
 * @brief Converts USX architecture ID into its canonical string representation.
 */
std::string resolve_architecture_name(uint16_t arch_id) noexcept {
    switch (arch_id) {
        case USX_ARCH_X86_64:  return "X86_64";
        case USX_ARCH_AARCH64: return "AARCH64";
        case USX_ARCH_RISCV64: return "RISCV64";
        default:               return "UNKNOWN";
    }
}

/**
 * @brief Scans frozen hash index buckets and categorizes origin and neo entries into sorted maps.
 */
void collect_index_entries(
    const NeoDatabase& db,
    std::map<std::string, const NeoIndexEntry*>& origin_entries,
    std::map<std::string, const NeoIndexEntry*>& neo_entries)
{
    const auto& hdr = db.header();
    const auto* buckets = reinterpret_cast<const NeoIndexEntry*>(db.raw_bytes().data() + hdr.index_offset);

    for (uint32_t i = 0; i < hdr.index_bucket_count; ++i) {
        const auto& entry = buckets[i];
        if (entry.name_hash == 0 || entry.name_str_offset >= hdr.strtab_size) {
            continue;
        }

        const char* raw_str = db.get_string(entry.name_str_offset);
        if (!raw_str) continue;

        const std::string_view name(raw_str);
        if (name == "__neo_meta__") continue;

        if (name.starts_with("origin(") && name.ends_with(")")) {
            const std::string origin_name(name.substr(7, name.size() - 8));
            origin_entries[origin_name] = &entry;
        } else if ((entry.flags & 1) == 0) {
            neo_entries[std::string(name)] = &entry;
        }
    }
}

/**
 * @brief Maps native instruction template IDs to origin opcode mnemonic names.
 */
std::unordered_map<uint16_t, std::string> build_id_to_origin_map(
    const NeoDatabase& db,
    const std::map<std::string, const NeoIndexEntry*>& origin_entries)
{
    std::unordered_map<uint16_t, std::string> id_map;
    for (const auto& [name, entry_ptr] : origin_entries) {
        const auto tmpls = db.get_templates(*entry_ptr);
        if (!tmpls.empty()) {
            id_map[tmpls[0].instruction_id] = name;
        }
    }
    return id_map;
}

/**
 * @brief Emits root architecture metadata keys into YAML emitter.
 */
void emit_header_metadata(YAML::Emitter& out, const NeoDbHeader& hdr) {
    const std::string endian_str = (hdr.arch_flags & USX_AFLAG_BIG_ENDIAN) ? "big" : "little";
    const uint16_t native_regs = (hdr.arch_flags & USX_AFLAG_REG_COUNT) ? 32 : 16;

    out << YAML::BeginMap;
    out << YAML::Key << "arch" << YAML::Value << resolve_architecture_name(hdr.arch_id);
    out << YAML::Key << "arch_id" << YAML::Value << hdr.arch_id;
    out << YAML::Key << "endian" << YAML::Value << endian_str;
    out << YAML::Key << "registers" << YAML::Value << 32;
    out << YAML::Key << "native_registers" << YAML::Value << native_regs;
    out << YAML::Key << "pic_mode" << YAML::Value << true;
}

} // namespace

bool decompile_neodb_to_yaml(const std::filesystem::path& neodb_in,
                            const std::filesystem::path& yaml_out) {
    NeoDatabase db;
    if (!db.load_file(neodb_in)) {
        return false;
    }

    std::map<std::string, const NeoIndexEntry*> origin_entries;
    std::map<std::string, const NeoIndexEntry*> neo_entries;
    collect_index_entries(db, origin_entries, neo_entries);

    const auto id_to_origin = build_id_to_origin_map(db, origin_entries);

    YAML::Emitter out;
    emit_header_metadata(out, db.header());

    out << YAML::Key << "ops" << YAML::Value << YAML::BeginMap;
    emit_neo_section(out, db, neo_entries, id_to_origin, origin_entries);
    emit_origin_section(out, db, origin_entries);
    out << YAML::EndMap; // ops
    out << YAML::EndMap; // root map

    std::ofstream file(yaml_out);
    if (!file.is_open()) return false;
    file << out.c_str() << "\n";
    return file.good();
}

} // namespace neo
