/**
 * @file translator.hpp
 * @brief Bidirectional YAML <-> .neodb translator interface for Proto-V.
 */

#pragma once

#include <filesystem>

namespace neo {

/**
 * @brief Bidirectional translator converting human-readable architecture YAML
 * specifications into zero-copy binary databases (.neodb) and vice versa.
 */
class NeoTranslator {
public:
    /**
     * @brief Translates a YAML architecture specification into a .neodb binary database.
     *
     * Parses the YAML architecture specification, extracts architecture metadata,
     * builds deduplicated null-terminated string table (.strtab), serializes packed
     * 56-byte instruction templates (.tmpl), builds the 256-bucket frozen FNV-1a
     * hash index (.idx), aligns all sections to 8-byte boundaries, calculates the
     * 64-byte header CRC32, and writes the completed binary database.
     *
     * @param yaml_in Path to input human-readable YAML architecture specification.
     * @param neodb_out Path to output .neodb binary opcode database.
     * @return True if conversion succeeded, false on any parsing, alignment, or I/O failure.
     */
    static bool yaml_to_neodb(const std::filesystem::path& yaml_in,
                              const std::filesystem::path& neodb_out);

    /**
     * @brief Decompiles a .neodb binary opcode database back into formatted YAML.
     *
     * @param neodb_in Path to input .neodb binary database.
     * @param yaml_out Path to output human-readable YAML specification.
     * @return True if decompilation succeeded, false on validation or I/O failure.
     */
    static bool neodb_to_yaml(const std::filesystem::path& neodb_in,
                              const std::filesystem::path& yaml_out);
};

} // namespace neo
