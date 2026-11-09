/**
 * @file compiler_pipeline.hpp
 * @brief Multi-pass compiler orchestrator coordinating Pass 0 through Pass 4.
 */

#pragma once

#include "neo/database.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace neo::detail {

/**
 * @brief Result of compiling source through the 4-pass pipeline.
 */
struct PipelineResult {
    bool success{false};
    std::vector<std::string> errors;
};

/**
 * @brief Orchestrates Pass 0 (Preprocessor), Pass 1 (Parser), Pass 2 (Expansion),
 * Pass 3 (PIC Resolution), and Pass 4 (Bitfield Emission).
 */
class CompilerPipeline {
public:
    /**
     * @brief Executes the complete 5-pass compilation pipeline.
     * @param source Raw Neo-ASM source text.
     * @param db Mapped target architecture database.
     * @param out_bin Output byte vector for flat binary machine code.
     * @param current_file Optional source file path for relative includes.
     * @return PipelineResult with success status and any diagnostics.
     */
    static PipelineResult run(
        std::string_view source,
        const NeoDatabase& db,
        std::vector<uint8_t>& out_bin,
        const std::filesystem::path& current_file = "");
};

} // namespace neo::detail
