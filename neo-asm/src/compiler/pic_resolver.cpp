/**
 * @file pic_resolver.cpp
 * @brief Implementation of Position-Independent Code (PIC) branch displacement resolver.
 */

#include "neo/pic_resolver.hpp"

namespace neo {

namespace {

/**
 * @brief Verifies that branch target displacement adheres to architecture word alignment.
 */
bool verify_alignment(uint64_t target_pc, uint64_t current_pc, uint16_t arch_id) noexcept {
    const uint64_t diff = (target_pc >= current_pc) ? (target_pc - current_pc) : (current_pc - target_pc);
    if (arch_id == USX_ARCH_AARCH64) {
        return (diff & 0x03) == 0; // AArch64 requires 4-byte instruction alignment
    }
    if (arch_id == USX_ARCH_RISCV64) {
        return (diff & 0x01) == 0; // RISC-V requires 2-byte instruction alignment
    }
    return true; // x86_64 allows byte alignment for relative jumps
}

/**
 * @brief Checks if displacement fits in signed two's-complement bitfield capacity.
 */
bool fits_in_bit_width(int64_t disp, uint8_t bit_width) noexcept {
    if (bit_width == 0 || bit_width > 64) return false;
    if (bit_width == 64) return true;
    const int64_t min_val = -(static_cast<int64_t>(1) << (bit_width - 1));
    const int64_t max_val = (static_cast<int64_t>(1) << (bit_width - 1)) - 1;
    return (disp >= min_val && disp <= max_val);
}

} // namespace

void NeoPicResolver::record_label(std::string_view label_name, uint64_t pc) {
    if (label_name.empty()) return;
    if (label_name.front() == '@') label_name.remove_prefix(1);
    tag_positions_[std::string(label_name)] = pc;
}

void NeoPicResolver::set_instruction_pc(size_t instruction_index, uint64_t pc) {
    if (instruction_index >= instruction_pcs_.size()) {
        instruction_pcs_.resize(instruction_index + 1, 0);
    }
    instruction_pcs_[instruction_index] = pc;
}

void NeoPicResolver::scan_instruction_lengths(
    const std::vector<uint8_t>& instruction_lengths, uint64_t base_pc) {
    instruction_pcs_.clear();
    instruction_pcs_.reserve(instruction_lengths.size());
    uint64_t current_pc = base_pc;
    for (uint8_t len : instruction_lengths) {
        instruction_pcs_.push_back(current_pc);
        current_pc += len;
    }
}

void NeoPicResolver::import_symbol_table(const SymbolTable& symbols, uint64_t total_bytes) {
    for (const auto& [name, sym] : symbols.labels()) {
        const uint64_t pc = (sym.instruction_index < instruction_pcs_.size())
            ? instruction_pcs_[sym.instruction_index]
            : total_bytes;
        record_label(sym.full_path, pc);
        record_label(sym.name, pc);
    }
}

std::optional<uint64_t> NeoPicResolver::get_instruction_pc(size_t instruction_index) const noexcept {
    if (instruction_index >= instruction_pcs_.size()) return std::nullopt;
    return instruction_pcs_[instruction_index];
}

std::string NeoPicResolver::resolve_label_name(
    std::string_view label_name, std::string_view current_scope) const {
    if (label_name.empty()) return {};
    if (label_name.front() == '@') label_name.remove_prefix(1);
    if (has_label(label_name)) return std::string(label_name);

    if (!current_scope.empty()) {
        std::string_view scope = current_scope;
        while (true) {
            std::string candidate = std::string(scope) + "." + std::string(label_name);
            if (has_label(candidate)) return candidate;

            const auto dot_pos = scope.rfind('.');
            if (dot_pos == std::string_view::npos) break;
            scope = scope.substr(0, dot_pos);
        }
    }
    return {};
}

std::optional<uint64_t> NeoPicResolver::get_label_pc(
    std::string_view label_name, std::string_view current_scope) const {
    const std::string resolved = resolve_label_name(label_name, current_scope);
    if (resolved.empty()) return std::nullopt;
    const auto it = tag_positions_.find(resolved);
    return (it != tag_positions_.end()) ? std::optional<uint64_t>(it->second) : std::nullopt;
}

bool NeoPicResolver::has_label(std::string_view label_name) const noexcept {
    if (label_name.empty()) return false;
    if (label_name.front() == '@') label_name.remove_prefix(1);
    return tag_positions_.find(std::string(label_name)) != tag_positions_.end();
}

void NeoPicResolver::clear() noexcept {
    tag_positions_.clear();
    instruction_pcs_.clear();
}

PicDisplacementResult NeoPicResolver::calculate_displacement(
    uint64_t target_pc, uint64_t current_pc, uint8_t insn_length,
    uint8_t bit_width, uint16_t arch_id) noexcept {
    if (!verify_alignment(target_pc, current_pc, arch_id)) {
        return {.success = false, .target_pc = target_pc, .error_code = PicErrorCode::InvalidAlignment,
                .error_message = "Branch displacement violates architecture alignment rules"};
    }

    int64_t disp = 0;
    if (arch_id == USX_ARCH_X86_64) {
        // x86 relative jumps are computed from end of jump instruction
        disp = static_cast<int64_t>(target_pc) - static_cast<int64_t>(current_pc + insn_length);
    } else if (arch_id == USX_ARCH_AARCH64) {
        // AArch64 branches encode 4-byte instruction word offsets
        disp = (static_cast<int64_t>(target_pc) - static_cast<int64_t>(current_pc)) >> 2;
    } else if (arch_id == USX_ARCH_RISCV64) {
        // RISC-V relative branches encode byte offsets from instruction PC
        disp = static_cast<int64_t>(target_pc) - static_cast<int64_t>(current_pc);
    } else {
        return {.success = false, .target_pc = target_pc, .error_code = PicErrorCode::UnsupportedArchitecture,
                .error_message = "Unsupported target architecture ID"};
    }

    if (!fits_in_bit_width(disp, bit_width)) {
        return {.success = false, .displacement = disp, .target_pc = target_pc,
                .error_code = PicErrorCode::DisplacementOutOfRange,
                .error_message = "Displacement " + std::to_string(disp) + " exceeds bitfield capacity"};
    }

    return {.success = true, .displacement = disp, .target_pc = target_pc,
            .error_code = PicErrorCode::Success, .error_message = ""};
}

PicDisplacementResult NeoPicResolver::resolve_displacement(
    std::string_view target_label, uint64_t current_pc, uint8_t insn_length,
    uint8_t bit_width, uint16_t arch_id, std::string_view current_scope) const {
    const auto target_pc = get_label_pc(target_label, current_scope);
    if (!target_pc.has_value()) {
        return {.success = false, .target_pc = 0, .error_code = PicErrorCode::UndefinedLabel,
                .error_message = "Undefined branch target label: '" + std::string(target_label) + "'"};
    }
    return calculate_displacement(*target_pc, current_pc, insn_length, bit_width, arch_id);
}

PicDisplacementResult NeoPicResolver::resolve_displacement(
    std::string_view target_label, uint64_t current_pc, uint8_t insn_length,
    const NeoOperandSlot& slot, uint16_t arch_id, std::string_view current_scope) const {
    return resolve_displacement(target_label, current_pc, insn_length, slot.bit_width, arch_id, current_scope);
}

PicDisplacementResult NeoPicResolver::resolve_displacement_for_instruction(
    size_t instruction_index, std::string_view target_label, uint8_t insn_length,
    uint8_t bit_width, uint16_t arch_id, std::string_view current_scope) const {
    const auto pc = get_instruction_pc(instruction_index);
    if (!pc.has_value()) {
        return {.success = false, .error_code = PicErrorCode::UndefinedLabel,
                .error_message = "Unrecorded instruction PC at index " + std::to_string(instruction_index)};
    }
    return resolve_displacement(target_label, *pc, insn_length, bit_width, arch_id, current_scope);
}

} // namespace neo
