/**
 * @file preprocessor.hpp
 * @brief Source preprocessor interface for Neo-ASM supporting directives.
 */

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace neo {

/**
 * @brief Preprocessor configuration, architecture target, and symbol context.
 */
struct PreprocessorContext {
    std::string target_arch{"x86_64"};
#if defined(_WIN32)
    std::string target_os{"windows"};
#else
    std::string target_os{"linux"};
#endif
    std::unordered_map<std::string, std::string> symbols;
    std::vector<std::filesystem::path> include_dirs;
    std::unordered_set<std::string> visited_files;
    std::unordered_set<std::string> active_includes;
};

/**
 * @brief Origin mapping for preprocessed source lines.
 */
struct SourceOrigin {
    std::string file_path;
    uint32_t line_number{1};
};

/**
 * @brief Result produced by NeoPreprocessor.
 */
struct PreprocessResult {
    bool success{false};
    std::string output_text;
    std::vector<std::string> errors;
    std::vector<SourceOrigin> line_mappings;
};

/**
 * @brief Preprocessor for Neo-ASM source text.
 *
 * Handles file inclusion (@INCLUDE), conditional compilation (@IF, @IFNDEF,
 * @IFDEF, @ELSE, @ELIF, @ENDIF), and predefined/user symbols.
 */
class NeoPreprocessor {
public:
    /**
     * @brief Constructs preprocessor with optional context.
     * @param context Preprocessor configuration and symbols.
     */
    explicit NeoPreprocessor(PreprocessorContext context = {});

    /**
     * @brief Preprocesses Neo-ASM source string.
     * @param source Raw Neo-ASM source code.
     * @param current_file Path of current file for relative includes.
     * @return PreprocessResult containing preprocessed text or diagnostics.
     */
    [[nodiscard]] PreprocessResult process(
        std::string_view source,
        const std::filesystem::path& current_file = "");

    /**
     * @brief Preprocesses Neo-ASM file from disk.
     * @param file_path Path of source file to load and process.
     * @return PreprocessResult containing preprocessed text or diagnostics.
     */
    [[nodiscard]] PreprocessResult process_file(
        const std::filesystem::path& file_path);

    /**
     * @brief Defines a preprocessor symbol.
     * @param name Symbol name.
     * @param value Symbol replacement value.
     */
    void define_symbol(std::string_view name, std::string_view value = "1");

    /**
     * @brief Undefines an existing preprocessor symbol.
     * @param name Symbol name.
     */
    void undefine_symbol(std::string_view name);

    /**
     * @brief Checks if a symbol is currently defined.
     * @param name Symbol name.
     * @return True if defined, false otherwise.
     */
    [[nodiscard]] bool is_defined(std::string_view name) const noexcept;

    /**
     * @brief Retrieves the value of a defined symbol.
     * @param name Symbol name.
     * @return Symbol value or empty string if undefined.
     */
    [[nodiscard]] std::string get_symbol_value(std::string_view name) const;

    /**
     * @brief Const reference to context.
     */
    [[nodiscard]] const PreprocessorContext& context() const noexcept {
        return context_;
    }

    /**
     * @brief Mutable reference to context.
     */
    [[nodiscard]] PreprocessorContext& context() noexcept {
        return context_;
    }

    /**
     * @brief Convenience static execution helper.
     * @param source Raw Neo-ASM source code.
     * @param current_file Path of current file.
     * @param context Preprocessor context.
     * @return PreprocessResult.
     */
    static PreprocessResult run(
        std::string_view source,
        const std::filesystem::path& current_file,
        const PreprocessorContext& context);

private:
    void init_predefined_symbols();

    PreprocessorContext context_;
};

} // namespace neo
