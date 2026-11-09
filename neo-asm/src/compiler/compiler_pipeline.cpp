/**
 * @file compiler_pipeline.cpp
 * @brief Implementation of 5-pass Neo-ASM compilation pipeline coordinator.
 */

#include "compiler_pipeline.hpp"
#include "compiler_instruction_planner.hpp"
#include "neo/emitter.hpp"
#include "neo/parser.hpp"
#include "neo/pic_resolver.hpp"
#include "neo/preprocessor.hpp"
#include "neo/register_alloc.hpp"

namespace neo::detail {

namespace {

bool resolve_pic_displacements(
    std::vector<ConcreteInstruction>& instructions,
    const SymbolTable& symbols,
    const std::vector<size_t>& ast_to_concrete,
    uint16_t arch_id,
    std::vector<std::string>& errors) {
    std::vector<uint8_t> lengths;
    lengths.reserve(instructions.size());
    uint64_t total_bytes = 0;
    for (const auto& ci : instructions) {
        lengths.push_back(ci.tmpl.byte_length);
        total_bytes += ci.tmpl.byte_length;
    }
    NeoPicResolver resolver;
    resolver.scan_instruction_lengths(lengths, 0);

    for (const auto& [name, sym] : symbols.labels()) {
        uint64_t pc = total_bytes;
        if (sym.instruction_index < ast_to_concrete.size()) {
            const size_t ci_idx = ast_to_concrete[sym.instruction_index];
            if (const auto inst_pc = resolver.get_instruction_pc(ci_idx)) pc = *inst_pc;
        }
        resolver.record_label(sym.full_path, pc);
        resolver.record_label(sym.name, pc);
    }

    for (size_t i = 0; i < instructions.size(); ++i) {
        auto& ci = instructions[i];
        if (!ci.has_pic_branch) continue;
        const uint64_t cur_pc = resolver.get_instruction_pc(i).value_or(0);
        const auto disp = resolver.resolve_displacement(
            ci.branch_target_label, cur_pc, ci.tmpl.byte_length,
            ci.tmpl.slots[ci.pic_slot_index], arch_id, ci.branch_scope);
        if (!disp.success) {
            errors.push_back("PIC displacement error: " + disp.error_message);
            return false;
        }
        ci.slot_values[ci.pic_slot_index] = disp.displacement;
    }
    return true;
}

} // namespace

PipelineResult CompilerPipeline::run(
    std::string_view source,
    const NeoDatabase& db,
    std::vector<uint8_t>& out_bin,
    const std::filesystem::path& current_file) {
    out_bin.clear();
    const uint16_t arch_id = db.header().arch_id;

    // Pass 0: Preprocessor (@INCLUDE, @IF, @IFNDEF, conditional directives)
    PreprocessorContext p_ctx;
    p_ctx.target_arch = (arch_id == USX_ARCH_AARCH64) ? "aarch64" : (arch_id == USX_ARCH_RISCV64 ? "riscv64" : "x86_64");
    if (!current_file.empty()) p_ctx.include_dirs.push_back(current_file.parent_path());
    const auto prep_res = NeoPreprocessor(p_ctx).process(source, current_file);
    if (!prep_res.success) return {false, prep_res.errors};

    // Pass 1: Parser (Tokens to AST instructions and hierarchical labels)
    const auto parse_res = NeoParser::parse(prep_res.output_text);
    if (!parse_res.success) return {false, parse_res.errors};

    // Pass 2: Target expansion, register mapping, and scratch spilling
    std::vector<ConcreteInstruction> concrete;
    std::vector<size_t> ast_to_concrete;
    std::vector<std::string> errors;

    for (size_t i = 0; i < parse_res.instructions.size(); ++i) {
        ast_to_concrete.push_back(concrete.size());
        const auto expanded = NeoRegisterAllocator::expand_instruction(parse_res.instructions[i], arch_id);
        for (const auto& exp : expanded) {
            if (!plan_instruction(db, exp, parse_res.instructions[i].scope, i, arch_id, concrete, errors)) {
                return {false, errors};
            }
        }
    }

    // Pass 3: PIC Displacement resolution without relocation tables
    if (!resolve_pic_displacements(concrete, parse_res.symbols, ast_to_concrete, arch_id, errors)) {
        return {false, errors};
    }

    // Pass 4: Bitfield slot injection and flat machine code binary emission
    for (const auto& ci : concrete) {
        if (!NeoEmitter::emit_instruction(ci.tmpl, ci.slot_values, arch_id, out_bin)) {
            return {false, {"Failed to emit instruction opcode"}};
        }
    }
    return {true, {}};
}

} // namespace neo::detail
