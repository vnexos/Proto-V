/**
 * @file preprocessor.cpp
 * @brief Implementation of Neo-ASM source preprocessor.
 */

#include "neo/preprocessor.hpp"
#include "preprocessor_directives.hpp"

#include <fstream>
#include <sstream>

namespace neo {

namespace {

struct ActiveIncludeGuard {
    std::unordered_set<std::string>& active_set;
    std::string path;
    bool inserted{false};

    ActiveIncludeGuard(std::unordered_set<std::string>& set, const std::filesystem::path& p)
        : active_set(set) {
        if (!p.empty()) {
            std::error_code ec;
            auto can = std::filesystem::canonical(p, ec);
            path = ec ? std::filesystem::weakly_canonical(p).string() : can.string();
            inserted = active_set.insert(path).second;
        }
    }

    ~ActiveIncludeGuard() {
        if (inserted && !path.empty()) {
            active_set.erase(path);
        }
    }
};

} // namespace

NeoPreprocessor::NeoPreprocessor(PreprocessorContext context)
    : context_(std::move(context)) {
    init_predefined_symbols();
}

void NeoPreprocessor::init_predefined_symbols() {
    context_.symbols["__os__"] = context_.target_os;
    context_.symbols["__arch__"] = context_.target_arch;
}

void NeoPreprocessor::define_symbol(std::string_view name, std::string_view value) {
    context_.symbols[std::string(name)] = std::string(value);
}

void NeoPreprocessor::undefine_symbol(std::string_view name) {
    context_.symbols.erase(std::string(name));
}

bool NeoPreprocessor::is_defined(std::string_view name) const noexcept {
    return context_.symbols.find(std::string(name)) != context_.symbols.end();
}

std::string NeoPreprocessor::get_symbol_value(std::string_view name) const {
    const auto it = context_.symbols.find(std::string(name));
    return (it != context_.symbols.end()) ? it->second : "";
}

PreprocessResult NeoPreprocessor::process_file(const std::filesystem::path& file_path) {
    std::ifstream file(file_path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return {false, {}, {DirectiveHandler::format_err(file_path, 1, "Failed to open source file")}, {}};
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return process(content, file_path);
}

PreprocessResult NeoPreprocessor::run(std::string_view source,
                                      const std::filesystem::path& current_file,
                                      const PreprocessorContext& context) {
    NeoPreprocessor pp(context);
    return pp.process(source, current_file);
}

PreprocessResult NeoPreprocessor::process(std::string_view source,
                                          const std::filesystem::path& current_file) {
    PreprocessResult result;
    ActiveIncludeGuard include_guard(context_.active_includes, current_file);
    DirectiveHandler handler(context_, current_file);
    std::vector<std::string> included_chunks;
    std::string primary_output;
    std::istringstream stream{std::string(source)};
    std::string line;
    uint32_t line_num = 0;

    while (std::getline(stream, line)) {
        line_num++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto [dir, args] = DirectiveHandler::split_directive(line);

        if (dir == "@IF" || dir == "@IFNDEF" || dir == "@IFDEF") {
            handler.handle_if(dir, args, line_num, result);
        } else if (dir == "@ELSE") {
            handler.handle_else(line_num, result);
        } else if (dir == "@ELIF" || dir == "@ELSEIF") {
            handler.handle_elif(args, line_num, result);
        } else if (dir == "@ENDIF") {
            handler.handle_endif(line_num, result);
        } else if (dir == "@INCLUDE") {
            handler.handle_include(args, line_num, result, included_chunks);
        } else if (dir == "@DEFINE") {
            handler.handle_define(args);
        } else if (dir == "@UNDEF") {
            handler.handle_undef(args);
        } else if (!dir.empty()) {
            if (handler.is_active()) {
                result.errors.push_back(DirectiveHandler::format_err(current_file, line_num, "Unknown directive " + dir));
            }
        } else if (handler.is_active()) {
            primary_output += line;
            primary_output += '\n';
            result.line_mappings.push_back({current_file.string(), line_num});
        }
    }

    handler.finalize(result);
    if (!result.errors.empty()) return result;

    result.success = true;
    result.output_text = primary_output;
    for (const auto& chunk : included_chunks) {
        if (!chunk.empty()) {
            if (!result.output_text.empty() && result.output_text.back() != '\n') result.output_text += '\n';
            result.output_text += chunk;
            if (result.output_text.back() != '\n') result.output_text += '\n';
        }
    }
    return result;
}

} // namespace neo
