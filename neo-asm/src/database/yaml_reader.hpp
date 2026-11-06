/**
 * @file yaml_reader.hpp
 * @brief Parses human-readable YAML architecture specifications into domain models.
 */

#pragma once

#include "yaml_model.hpp"

#include <filesystem>
#include <optional>

namespace neo {

/**
 * @brief Loads and parses a YAML architecture specification file.
 *
 * @param path Path to the human-readable YAML file.
 * @return ParsedArchitectureSpec on success, or std::nullopt on parsing or I/O failure.
 */
[[nodiscard]] std::optional<ParsedArchitectureSpec> load_yaml_architecture(
    const std::filesystem::path& path);

} // namespace neo
