/**
 * @file label_tracker.hpp
 * @brief Indentation-based hierarchical label scope tracker and resolution.
 */

#pragma once

#include "neo/parser.hpp"

#include <string>
#include <vector>

namespace neo {

/**
 * @brief Manages indentation-driven scope stacks for hierarchical label definitions.
 */
class LabelTracker {
public:
    LabelTracker() = default;

    /**
     * @brief Gets current active dotted scope path (e.g. "label_a.label_b").
     */
    [[nodiscard]] std::string current_scope() const;

    /**
     * @brief Pushes a newly defined label, adjusting scope based on indentation level.
     */
    std::string push_label(std::string_view name,
                           uint32_t indent,
                           SourceLocation loc,
                           SymbolTable& symbols);

    /**
     * @brief Binds pending label definitions to the next emitted instruction index.
     */
    void bind_pending_labels(size_t instruction_idx, SymbolTable& symbols);

    /**
     * @brief Binds any trailing labels at end of stream to the final instruction count.
     */
    void finalize(size_t total_instructions, SymbolTable& symbols);

private:
    struct ScopeEntry {
        uint32_t indent{0};
        std::string local_name;
        std::string full_path;
    };

    std::vector<ScopeEntry> stack_;
    std::vector<std::string> pending_labels_;
};

} // namespace neo
