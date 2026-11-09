/**
 * @file preprocessor_directives.cpp
 * @brief Implementation of preprocessor directive execution.
 */

#include "preprocessor_directives.hpp"

#include <cctype>

namespace neo {

DirectiveHandler::DirectiveHandler(PreprocessorContext& ctx, const std::filesystem::path& current_file)
    : context_(ctx), current_file_(current_file), eval_(ctx), inc_resolver_(ctx) {}

bool DirectiveHandler::is_active() const noexcept {
    for (const auto& frame : stack_) {
        if (!frame.is_active) return false;
    }
    return true;
}

bool DirectiveHandler::is_parent_active() const noexcept {
    if (stack_.size() <= 1) return true;
    for (size_t i = 0; i + 1 < stack_.size(); ++i) {
        if (!stack_[i].is_active) return false;
    }
    return true;
}

std::string DirectiveHandler::format_err(const std::filesystem::path& file, uint32_t line, std::string_view msg) {
    const std::string f = file.empty() ? "<source>" : file.string();
    return f + ":" + std::to_string(line) + ": error: " + std::string(msg);
}

std::pair<std::string, std::string> DirectiveHandler::split_directive(std::string_view line) {
    size_t start = 0;
    while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) start++;
    if (start >= line.size() || line[start] != '@') return {"", ""};
    size_t end = start + 1;
    while (end < line.size() && std::isalpha(static_cast<unsigned char>(line[end]))) end++;
    std::string dir(line.substr(start, end - start));
    for (char& c : dir) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    size_t arg_start = end;
    while (arg_start < line.size() && (line[arg_start] == ' ' || line[arg_start] == '\t')) arg_start++;
    return {dir, std::string(line.substr(arg_start))};
}

void DirectiveHandler::handle_if(std::string_view dir, std::string_view args, uint32_t line_num, PreprocessResult& res) {
    if (!is_active()) {
        stack_.push_back({true, false, false, line_num});
        return;
    }
    EvalResult r = (dir == "@IF") ? eval_.evaluate(args) :
                   (dir == "@IFNDEF" ? eval_.evaluate_ifndef(args) : eval_.evaluate_ifdef(args));
    if (!r.success) res.errors.push_back(format_err(current_file_, line_num, r.error));
    stack_.push_back({r.value, r.value, false, line_num});
}

void DirectiveHandler::handle_else(uint32_t line_num, PreprocessResult& res) {
    if (stack_.empty()) {
        res.errors.push_back(format_err(current_file_, line_num, "Unmatched @ELSE directive"));
        return;
    }
    if (stack_.back().else_seen) {
        res.errors.push_back(format_err(current_file_, line_num, "Duplicate @ELSE directive"));
        return;
    }
    stack_.back().else_seen = true;
    const bool can_activate = is_parent_active() && !stack_.back().condition_met;
    stack_.back().is_active = can_activate;
    if (can_activate) stack_.back().condition_met = true;
}

void DirectiveHandler::handle_elif(std::string_view args, uint32_t line_num, PreprocessResult& res) {
    if (stack_.empty()) {
        res.errors.push_back(format_err(current_file_, line_num, "Unmatched @ELIF directive"));
        return;
    }
    if (stack_.back().else_seen) {
        res.errors.push_back(format_err(current_file_, line_num, "@ELIF after @ELSE directive"));
        return;
    }
    if (is_parent_active() && !stack_.back().condition_met) {
        EvalResult r = eval_.evaluate(args);
        if (!r.success) res.errors.push_back(format_err(current_file_, line_num, r.error));
        stack_.back().is_active = r.value;
        if (r.value) stack_.back().condition_met = true;
    } else {
        stack_.back().is_active = false;
    }
}

void DirectiveHandler::handle_endif(uint32_t line_num, PreprocessResult& res) {
    if (stack_.empty()) {
        res.errors.push_back(format_err(current_file_, line_num, "Unmatched @ENDIF directive"));
    } else {
        stack_.pop_back();
    }
}

void DirectiveHandler::handle_include(std::string_view args, uint32_t line_num, PreprocessResult& res,
                                     std::vector<std::string>& included_chunks) {
    if (!is_active()) return;
    auto inc_res = inc_resolver_.resolve_and_read(args, current_file_, line_num);
    if (!inc_res.success) {
        res.errors.push_back(format_err(current_file_, line_num, inc_res.error));
        return;
    }
    NeoPreprocessor sub_pp(context_);
    auto sub_res = sub_pp.process(inc_res.content, inc_res.resolved_path);
    if (!sub_res.success) {
        for (auto& err : sub_res.errors) res.errors.push_back(std::move(err));
    } else {
        included_chunks.push_back(std::move(sub_res.output_text));
        for (const auto& [k, v] : sub_pp.context().symbols) context_.symbols[k] = v;
    }
}

void DirectiveHandler::handle_define(std::string_view args) {
    if (!is_active()) return;
    size_t start = 0;
    while (start < args.size() && std::isspace(static_cast<unsigned char>(args[start]))) start++;
    size_t end = start;
    while (end < args.size() && !std::isspace(static_cast<unsigned char>(args[end]))) end++;
    if (start == end) return;
    const std::string sym(args.substr(start, end - start));
    size_t val_start = end;
    while (val_start < args.size() && std::isspace(static_cast<unsigned char>(args[val_start]))) val_start++;
    size_t val_end = args.size();
    while (val_end > val_start && (std::isspace(static_cast<unsigned char>(args[val_end - 1])) || args[val_end - 1] == ';')) val_end--;
    const std::string val = (val_start < val_end) ? std::string(args.substr(val_start, val_end - val_start)) : "1";
    context_.symbols[sym] = val;
}

void DirectiveHandler::handle_undef(std::string_view args) {
    if (!is_active()) return;
    size_t s = 0;
    while (s < args.size() && std::isspace(static_cast<unsigned char>(args[s]))) s++;
    size_t e = args.size();
    while (e > s && std::isspace(static_cast<unsigned char>(args[e - 1]))) e--;
    if (s < e) context_.symbols.erase(std::string(args.substr(s, e - s)));
}

void DirectiveHandler::finalize(PreprocessResult& res) {
    if (!stack_.empty()) {
        res.errors.push_back(format_err(current_file_, stack_.back().line, "Unclosed @IF directive"));
    }
}

} // namespace neo
