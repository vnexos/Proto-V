/**
 * @file preprocessor_include.cpp
 * @brief Implementation of file inclusion resolution and circular detection.
 */

#include "preprocessor_include.hpp"

#include <fstream>

namespace neo {

IncludeResolver::IncludeResolver(PreprocessorContext& context) noexcept
    : context_(context) {}

std::string IncludeResolver::extract_path_spec(std::string_view args) {
    size_t start = 0;
    while (start < args.size() && (args[start] == ' ' || args[start] == '\t')) {
        start++;
    }

    if (start >= args.size()) {
        return "";
    }

    const char quote = args[start];
    if (quote == '"' || quote == '\'' || quote == '<') {
        const char close_quote = (quote == '<') ? '>' : quote;
        const size_t end = args.find(close_quote, start + 1);
        if (end == std::string_view::npos) {
            return "";
        }
        return std::string(args.substr(start + 1, end - start - 1));
    }

    size_t end = start;
    while (end < args.size() && args[end] != ' ' && args[end] != '\t' &&
           args[end] != ';' && args[end] != '\r' && args[end] != '\n') {
        end++;
    }

    return std::string(args.substr(start, end - start));
}

std::optional<std::filesystem::path> IncludeResolver::locate_file(
    const std::filesystem::path& target,
    const std::filesystem::path& current_file) const {
    if (target.is_absolute() && std::filesystem::exists(target)) {
        return target;
    }

    if (!current_file.empty()) {
        const auto parent = current_file.parent_path();
        const auto candidate = parent / target;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    const auto cwd_candidate = std::filesystem::current_path() / target;
    if (std::filesystem::exists(cwd_candidate)) {
        return cwd_candidate;
    }

    for (const auto& dir : context_.include_dirs) {
        const auto candidate = dir / target;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return std::nullopt;
}

IncludeResult IncludeResolver::resolve_and_read(
    std::string_view directive_args,
    const std::filesystem::path& current_file,
    uint32_t line_number) {
    (void)line_number;
    const std::string path_spec = extract_path_spec(directive_args);
    if (path_spec.empty()) {
        return {false, {}, {}, "Missing include file path"};
    }

    const auto located = locate_file(path_spec, current_file);
    if (!located.has_value()) {
        return {false, {}, {}, "Include file not found: '" + path_spec + "'"};
    }

    std::error_code ec;
    auto canonical = std::filesystem::canonical(*located, ec);
    if (ec) {
        canonical = std::filesystem::weakly_canonical(*located);
    }

    if (context_.active_includes.find(canonical.string()) != context_.active_includes.end()) {
        return {false, canonical, {}, "Circular inclusion detected: '" + path_spec + "'"};
    }

    std::ifstream file(canonical, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return {false, canonical, {}, "Failed to open include file: '" + path_spec + "'"};
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    context_.visited_files.insert(canonical.string());
    return {true, canonical, std::move(content), ""};
}

} // namespace neo
