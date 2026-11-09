/**
 * @file preprocessor_directives.hpp
 * @brief Directive handling and conditional stack management for Neo-ASM.
 */

#pragma once

#include "neo/preprocessor.hpp"
#include "preprocessor_eval.hpp"
#include "preprocessor_include.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace neo {

struct CondFrame {
    bool condition_met{false};
    bool is_active{false};
    bool else_seen{false};
    uint32_t line{0};
};

/**
 * @brief Coordinates directive execution and tracks conditional nesting.
 */
class DirectiveHandler {
public:
    DirectiveHandler(PreprocessorContext& ctx, const std::filesystem::path& current_file);

    void handle_if(std::string_view dir, std::string_view args, uint32_t line_num, PreprocessResult& res);
    void handle_else(uint32_t line_num, PreprocessResult& res);
    void handle_elif(std::string_view args, uint32_t line_num, PreprocessResult& res);
    void handle_endif(uint32_t line_num, PreprocessResult& res);
    void handle_include(std::string_view args, uint32_t line_num, PreprocessResult& res,
                        std::vector<std::string>& included_chunks);
    void handle_define(std::string_view args);
    void handle_undef(std::string_view args);

    [[nodiscard]] bool is_active() const noexcept;
    void finalize(PreprocessResult& res);

    static std::pair<std::string, std::string> split_directive(std::string_view line);
    static std::string format_err(const std::filesystem::path& file, uint32_t line, std::string_view msg);

private:
    [[nodiscard]] bool is_parent_active() const noexcept;

    PreprocessorContext& context_;
    std::filesystem::path current_file_;
    ConditionEvaluator eval_;
    IncludeResolver inc_resolver_;
    std::vector<CondFrame> stack_;
};

} // namespace neo
