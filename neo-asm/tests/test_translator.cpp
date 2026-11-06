/**
 * @file test_translator.cpp
 * @brief Unit and integration tests for NeoTranslator::yaml_to_neodb builder.
 */

#include <neo/common.hpp>
#include <neo/database.hpp>
#include <neo/schema.hpp>
#include <neo/translator.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef NEO_TEMPLATES_DIR
#define NEO_TEMPLATES_DIR "../../../ai-driven/templates"
#endif

namespace {

std::filesystem::path resolve_templates_dir() {
    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::path(NEO_TEMPLATES_DIR),
        std::filesystem::path("/home/tvdquang/Code/ai-driven/templates"),
        std::filesystem::current_path() / "ai-driven/templates",
        std::filesystem::current_path() / "../../../ai-driven/templates"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate / "neo_asm_x86_64.yaml")) {
            return candidate;
        }
    }
    return {};
}

void test_x86_64_translation(const std::filesystem::path& tmpl_dir) {
    const auto yaml_path = tmpl_dir / "neo_asm_x86_64.yaml";
    const auto out_path = std::filesystem::temp_directory_path() / "test_x86_64.neodb";

    // 1. Convert YAML to .neodb binary database
    assert(neo::NeoTranslator::yaml_to_neodb(yaml_path, out_path));
    assert(std::filesystem::exists(out_path));

    // 2. Validate converted file with NeoDatabase zero-copy reader
    neo::NeoDatabase db;
    assert(db.load_file(out_path));
    assert(db.is_loaded());

    // 3. Verify header integrity and USX alignment
    const auto& hdr = db.header();
    assert(hdr.format_version == 0x00010000);
    assert(hdr.arch_id == neo::USX_ARCH_X86_64);
    assert((hdr.arch_flags & neo::USX_AFLAG_REG_COUNT) == 0); // 16 native GPRs
    assert(hdr.strtab_offset % 8 == 0);
    assert(hdr.index_offset % 8 == 0);
    assert(hdr.template_offset % 8 == 0);
    assert(hdr.index_bucket_count == 256);
    assert(hdr.strtab_size > 0);
    assert(hdr.template_size > 0);

    // 4. Verify canonical Neo-ASM mnemonics are queryable
    const std::vector<std::string_view> neo_mnemonics = {
        "ASN", "PUSH", "POP", "ADD", "SUB", "MUL", "DIV", "MOD", "NEG",
        "ADD32", "SUB32", "AND", "OR", "XOR", "NOT", "SHL", "SHR", "SAR",
        "CLZ", "JMP", "JEQ", "JNE", "JGT", "JGE", "JLT", "JLE", "CALL",
        "RET", "SYS", "BRK", "NOP", "FENCE", "UDIV", "UMOD", "LEA"
    };

    for (const auto mnem : neo_mnemonics) {
        const auto* entry = db.find_entry(mnem);
        assert(entry != nullptr);
        assert(entry->template_count >= 1);
        const auto tmpls = db.get_templates(*entry);
        assert(tmpls.size() == entry->template_count);
    }

    // 5. Verify multi-instruction expansion for ADD (MOV dst, src1 + ADD dst, src2)
    const auto* add_entry = db.find_entry("ADD");
    assert(add_entry != nullptr);
    assert(add_entry->template_count == 2);
    assert((add_entry->flags & 2) != 0); // IsMultiInstruction flag

    // 6. Verify native origin commands are queryable
    const auto* orig_mov = db.find_entry("origin(MOV)");
    assert(orig_mov != nullptr);
    assert(orig_mov->template_count == 1);
    assert((orig_mov->flags & 1) != 0); // IsOriginQuery flag

    const auto* orig_sys = db.find_entry("origin(SYSCALL)");
    assert(orig_sys != nullptr);
    const auto sys_tmpls = db.get_templates(*orig_sys);
    assert(sys_tmpls.size() == 1);
    assert(sys_tmpls[0].byte_length == 2);
    assert(sys_tmpls[0].opcode_bytes[0] == 0x0F && sys_tmpls[0].opcode_bytes[1] == 0x05);

    // Bare native command query should also succeed
    const auto* bare_mov = db.find_entry("MOV");
    assert(bare_mov != nullptr);

    std::filesystem::remove(out_path);
}

void test_aarch64_translation(const std::filesystem::path& tmpl_dir) {
    const auto yaml_path = tmpl_dir / "neo_asm_aarch64.yaml";
    const auto out_path = std::filesystem::temp_directory_path() / "test_aarch64.neodb";

    assert(neo::NeoTranslator::yaml_to_neodb(yaml_path, out_path));
    assert(std::filesystem::exists(out_path));

    neo::NeoDatabase db;
    assert(db.load_file(out_path));
    assert(db.is_loaded());

    const auto& hdr = db.header();
    assert(hdr.arch_id == neo::USX_ARCH_AARCH64);
    assert((hdr.arch_flags & neo::USX_AFLAG_REG_COUNT) != 0); // 32 native GPRs
    assert(hdr.strtab_offset % 8 == 0 && hdr.index_offset % 8 == 0 && hdr.template_offset % 8 == 0);

    // AArch64 ADD maps 1:1 to single native instruction
    const auto* add_entry = db.find_entry("ADD");
    assert(add_entry != nullptr && add_entry->template_count == 1);

    const auto* orig_svc = db.find_entry("origin(SVC)");
    assert(orig_svc != nullptr);
    const auto svc_tmpls = db.get_templates(*orig_svc);
    assert(svc_tmpls.size() == 1);
    assert(svc_tmpls[0].byte_length == 4);
    assert(svc_tmpls[0].opcode_bytes[0] == 0xD4 && svc_tmpls[0].opcode_bytes[3] == 0x01);

    std::filesystem::remove(out_path);
}

void test_riscv64_translation(const std::filesystem::path& tmpl_dir) {
    const auto yaml_path = tmpl_dir / "neo_asm_riscv64.yaml";
    const auto out_path = std::filesystem::temp_directory_path() / "test_riscv64.neodb";

    assert(neo::NeoTranslator::yaml_to_neodb(yaml_path, out_path));
    assert(std::filesystem::exists(out_path));

    neo::NeoDatabase db;
    assert(db.load_file(out_path));
    assert(db.is_loaded());

    const auto& hdr = db.header();
    assert(hdr.arch_id == neo::USX_ARCH_RISCV64);
    assert((hdr.arch_flags & neo::USX_AFLAG_REG_COUNT) != 0); // 32 native GPRs
    assert(hdr.strtab_offset % 8 == 0 && hdr.index_offset % 8 == 0 && hdr.template_offset % 8 == 0);

    const auto* ecall_entry = db.find_entry("origin(ECALL)");
    assert(ecall_entry != nullptr);
    const auto ecall_tmpls = db.get_templates(*ecall_entry);
    assert(ecall_tmpls.size() == 1 && ecall_tmpls[0].byte_length == 4);

    std::filesystem::remove(out_path);
}

void test_error_handling() {
    const auto temp_out = std::filesystem::temp_directory_path() / "test_err.neodb";

    // Non-existent YAML file
    assert(!neo::NeoTranslator::yaml_to_neodb("non_existent_file_xyz.yaml", temp_out));

    // Malformed YAML content
    const auto bad_yaml = std::filesystem::temp_directory_path() / "bad_syntax.yaml";
    {
        std::ofstream out(bad_yaml);
        out << "arch: [unclosed list\n";
    }
    assert(!neo::NeoTranslator::yaml_to_neodb(bad_yaml, temp_out));
    std::filesystem::remove(bad_yaml);

    // Invalid output path
    const auto invalid_out = "/dev/null/cannot_write_here/neodb.bin";
    assert(!neo::NeoTranslator::yaml_to_neodb(bad_yaml, invalid_out));
}

} // namespace

int main() {
    const auto tmpl_dir = resolve_templates_dir();
    assert(!tmpl_dir.empty());

    test_x86_64_translation(tmpl_dir);
    test_aarch64_translation(tmpl_dir);
    test_riscv64_translation(tmpl_dir);
    test_error_handling();

    std::cout << "[PASS] test_translator completed successfully.\n";
    return 0;
}
