/**
 * @file preprocessor_include.hpp
 * @brief Include resolution and circular dependency tracking for Neo-ASM.
 */

#pragma once

#include "neo/preprocessor.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace neo {

/**
 * @brief Result of include directive file lookup and loading.
 */
struct IncludeResult {
    bool success{false};
    std::filesystem::path resolved_path;
    std::string content;
    std::string error;
};

/**
 * @brief Resolves @INCLUDE file paths, reads content, and prevents recursion cycles.
 */
class IncludeResolver {
public:
    /**
     * @brief Constructs resolver referencing preprocessor context.
     * @param context Preprocessor context storing search paths and visited records.
     */
    explicit IncludeResolver(PreprocessorContext& context) noexcept;

    /**
     * @brief Resolves and reads included file content.
     * @param directive_args Raw arguments string following @INCLUDE.
     * @param current_file Path of file containing the directive.
     * @param line_number Line number where directive appeared.
     * @return IncludeResult with status, content, or diagnostic error.
     */
    [[nodiscard]] IncludeResult resolve_and_read(
        std::string_view directive_args,
        const std::filesystem::path& current_file,
        uint32_t line_number);

    /**
     * @brief Extracts cleaned path string from quotes or brackets.
     * @param args Raw string after directive.
     * @return Extracted path or empty string.
     */
    [[nodiscard]] static std::string extract_path_spec(std::string_view args);

private:
    [[nodiscard]] std::optional<std::filesystem::path> locate_file(
        const std::filesystem::path& target,
        const std::filesystem::path& current_file) const;

    PreprocessorContext& context_;
};

} // namespace neo
