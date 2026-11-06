/**
 * @file yaml_decompiler.hpp
 * @brief Decompiles .neodb binary opcode databases back into human-readable YAML.
 */

#pragma once

#include <filesystem>

namespace neo {

/**
 * @brief Decompiles an 8-byte aligned .neodb database back into human-readable YAML.
 *
 * Traverses index entries and packed instruction templates from a zero-copy NeoDatabase,
 * reconstructs canonical unified Neo-ASM and native origin opcode specifications, formats
 * all bitmasks with strict 0x%08X and opcodes with %02X, and sorts all instructions
 * deterministically by name to ensure lossless bit-identical roundtrips.
 *
 * @param neodb_in Path to input .neodb binary file.
 * @param yaml_out Path to output YAML specification file.
 * @return True on successful decompilation and emission, false on validation or I/O failure.
 */
bool decompile_neodb_to_yaml(const std::filesystem::path& neodb_in,
                            const std::filesystem::path& yaml_out);

} // namespace neo
