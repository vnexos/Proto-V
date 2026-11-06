/**
 * @file database_writer.hpp
 * @brief Serializes parsed architecture specifications into binary .neodb format.
 */

#pragma once

#include "yaml_model.hpp"

#include <filesystem>

namespace neo {

/**
 * @brief Serializes a ParsedArchitectureSpec into an 8-byte aligned, frozen-hash indexed .neodb file.
 *
 * @param spec Parsed architecture specification with metadata and instruction sets.
 * @param output_path Target file path for the .neodb binary.
 * @return True on successful binary emission, false on validation or I/O failure.
 */
bool write_neodb_file(const ParsedArchitectureSpec& spec,
                      const std::filesystem::path& output_path);

} // namespace neo
