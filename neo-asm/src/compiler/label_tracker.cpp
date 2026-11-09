/**
 * @file label_tracker.cpp
 * @brief Implementation of hierarchical label scope tracking and symbol resolution.
 */

#include "label_tracker.hpp"

namespace neo {

void SymbolTable::add_label(const LabelSymbol& symbol) {
    labels_[symbol.full_path] = symbol;
}

const LabelSymbol* SymbolTable::find_exact(std::string_view full_path) const noexcept {
    const auto it = labels_.find(std::string(full_path));
    return (it != labels_.end()) ? &it->second : nullptr;
}

bool SymbolTable::contains(std::string_view full_path) const noexcept {
    return labels_.find(std::string(full_path)) != labels_.end();
}

const std::unordered_map<std::string, LabelSymbol>& SymbolTable::labels() const noexcept {
    return labels_;
}

std::string SymbolTable::resolve_label(std::string_view target,
                                       std::string_view current_scope) const {
    if (target.empty()) return {};
    if (target.front() == '@') target.remove_prefix(1);

    // 1. Direct exact match on fully qualified path or top-level label
    if (contains(target)) return std::string(target);

    // 2. Ascend scope hierarchy searching for matching nested or sibling label
    if (!current_scope.empty()) {
        std::string_view scope = current_scope;
        while (true) {
            std::string candidate = std::string(scope) + "." + std::string(target);
            if (contains(candidate)) return candidate;

            const auto dot_pos = scope.rfind('.');
            if (dot_pos == std::string_view::npos) break;
            scope = scope.substr(0, dot_pos);
        }
    }
    return {};
}

std::string LabelTracker::current_scope() const {
    return stack_.empty() ? "" : stack_.back().full_path;
}

std::string LabelTracker::push_label(std::string_view name,
                                     uint32_t indent,
                                     SourceLocation loc,
                                     SymbolTable& symbols) {
    // Pop deeper or equal indentation scopes
    while (!stack_.empty() && stack_.back().indent >= indent) {
        stack_.pop_back();
    }

    const std::string parent = current_scope();
    const std::string local(name);
    const std::string full = parent.empty() ? local : (parent + "." + local);

    stack_.push_back({indent, local, full});
    pending_labels_.push_back(full);

    symbols.add_label(LabelSymbol{local, full, parent, indent, 0, loc});
    return full;
}

void LabelTracker::bind_pending_labels(size_t instruction_idx, SymbolTable& symbols) {
    for (const auto& full_path : pending_labels_) {
        if (auto* sym = const_cast<LabelSymbol*>(symbols.find_exact(full_path))) {
            sym->instruction_index = instruction_idx;
        }
    }
    pending_labels_.clear();
}

void LabelTracker::finalize(size_t total_instructions, SymbolTable& symbols) {
    bind_pending_labels(total_instructions, symbols);
}

} // namespace neo
