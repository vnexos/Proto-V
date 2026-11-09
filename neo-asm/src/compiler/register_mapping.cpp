/**
 * @file register_mapping.cpp
 * @brief Implementations of architecture-specific register translation routines.
 */

#include "register_mapping.hpp"

#include <array>
#include <string>

namespace neo::detail {

namespace {

struct PinnedReg {
    uint8_t id;
    const char* name;
};

// Fixed mapping for x86_64 primary general registers r0..r13
constexpr std::array<PinnedReg, 14> X86_PINNED = {{
    {0, "RAX"},  {1, "RCX"},  {2, "RDX"},  {3, "RBX"},
    {6, "RSI"},  {7, "RDI"},  {8, "R8"},   {9, "R9"},
    {10, "R10"}, {11, "R11"}, {12, "R12"}, {13, "R13"},
    {14, "R14"}, {15, "R15"}
}};

RegisterMapping map_aarch64_system(uint8_t reg) {
    if (reg == REG_SP) return {31, false, 0, "SP"};
    if (reg == REG_FP) return {29, false, 0, "X29"};
    if (reg == REG_ZR) return {31, false, 0, "XZR"};
    return {REG_INVALID, false, 0, "INVALID"};
}

RegisterMapping map_riscv64_system(uint8_t reg) {
    if (reg == REG_SP) return {2, false, 0, "x2"};
    if (reg == REG_FP) return {8, false, 0, "x8"};
    if (reg == REG_ZR) return {0, false, 0, "x0"};
    return {REG_INVALID, false, 0, "INVALID"};
}

RegisterMapping map_x86_64_system(uint8_t reg) {
    if (reg == REG_SP) return {4, false, 0, "RSP"};
    if (reg == REG_FP) return {5, false, 0, "RBP"};
    if (reg == REG_ZR) return {REG_INVALID, false, 0, "0"};
    return {REG_INVALID, false, 0, "INVALID"};
}

} // namespace

RegisterMapping map_aarch64(uint8_t reg, bool is_system) {
    if (is_system) {
        return map_aarch64_system(reg);
    }
    // r31 is remapped to callee-saved X19 per specification
    if (reg == 31) {
        return {19, false, 0, "X19"};
    }
    if (reg <= 30) {
        return {reg, false, 0, "X" + std::to_string(reg)};
    }
    return {REG_INVALID, false, 0, "INVALID"};
}

RegisterMapping map_riscv64(uint8_t reg, bool is_system) {
    if (is_system) {
        return map_riscv64_system(reg);
    }
    if (reg <= 31) {
        return {reg, false, 0, "x" + std::to_string(reg)};
    }
    return {REG_INVALID, false, 0, "INVALID"};
}

RegisterMapping map_x86_64(uint8_t reg, bool is_system) {
    if (is_system) {
        return map_x86_64_system(reg);
    }
    if (reg < X86_PINNED.size()) {
        const auto& pinned = X86_PINNED[reg];
        return {pinned.id, false, 0, pinned.name};
    }
    if (reg <= 31) {
        const int32_t offset = NeoRegisterAllocator::spill_offset(reg);
        return {REG_INVALID, true, offset, "r" + std::to_string(reg)};
    }
    return {REG_INVALID, false, 0, "INVALID"};
}

} // namespace neo::detail
