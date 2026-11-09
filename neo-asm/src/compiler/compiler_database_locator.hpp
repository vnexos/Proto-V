/**
 * @file compiler_database_locator.hpp
 * @brief Locates and loads opcode database (.neodb) for specified target architecture.
 */

#pragma once

#include "neo/database.hpp"

#include <filesystem>
#include <memory>
#include <string_view>

namespace neo::detail {

/**
 * @brief Resolves and loads the NeoDatabase for a target architecture.
 *
 * Checks explicit paths, standard search paths, and automatically compiles
 * template YAML definitions on-the-fly to temporary databases if needed.
 *
 * @param arch Target architecture identifier string.
 * @param custom_db_path Optional explicit path to .neodb file.
 * @return Loaded NeoDatabase instance, or nullptr if resolution failed.
 */
std::unique_ptr<NeoDatabase> locate_database(
    std::string_view arch,
    const std::filesystem::path& custom_db_path = "");

} // namespace neo::detail
