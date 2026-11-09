/**
 * @file test_cli_workflows.cpp
 * @brief Implementation of end-to-end integration and compilation workflows for neo-asm CLI.
 */

#include "test_cli_workflows.hpp"
#include "test_cli_process.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace neo::test {

namespace {

std::vector<uint8_t> read_binary_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void test_database_installation(const std::string& exe, const std::filesystem::path& tmpl_dir) {
    const auto work_dir = std::filesystem::temp_directory_path() / "cli_db_inst_suite";
    std::filesystem::create_directories(work_dir);

    const auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(work_dir);

    const auto yaml_in = tmpl_dir / "neo_asm_x86_64.yaml";
    const auto dummy_neodb = work_dir / "install_target.neodb";
    assert(run_process(exe + " -t \"" + yaml_in.string() + "\" \"" + dummy_neodb.string() + "\"").exit_code == 0);

    assert(run_process(exe + " -idb \"" + dummy_neodb.string() + "\"").exit_code == 0);
    assert(std::filesystem::exists(work_dir / "database" / "install_target.neodb"));
    assert(run_process(exe + " --install-database \"" + dummy_neodb.string() + "\"").exit_code == 0);

    std::filesystem::current_path(prev_cwd);
    std::error_code ec;
    std::filesystem::remove_all(work_dir, ec);
    std::cout << "[PASS] test_database_installation\n";
}

void test_database_resolution_and_preprocessor(const std::string& exe, const std::filesystem::path& tmpl_dir) {
    const auto work_dir = std::filesystem::temp_directory_path() / "test_cli_db_res";
    std::filesystem::create_directories(work_dir);

    const auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(work_dir);

    const auto yaml_x86 = tmpl_dir / "neo_asm_x86_64.yaml";
    const auto neodb_x86 = work_dir / "x86_64.neodb";
    assert(run_process(exe + " -t \"" + yaml_x86.string() + "\" \"" + neodb_x86.string() + "\"").exit_code == 0);
    assert(run_process(exe + " -idb \"" + neodb_x86.string() + "\"").exit_code == 0);
    assert(std::filesystem::exists(work_dir / "database" / "x86_64.neodb"));

    const auto inc_path = work_dir / "inc.neoa";
    {
        std::ofstream out(inc_path);
        out << "origin(SYSCALL)\n";
    }

    const auto app_path = work_dir / "app.neoa";
    {
        std::ofstream out(app_path);
        out << "@IF __arch__ == \"x86_64\"\n";
        out << "@INCLUDE \"inc.neoa\"\n";
        out << "@ENDIF\n";
        out << "@IFNDEF NON_EXISTENT_FEATURE\n";
        out << "origin(SYSCALL)\n";
        out << "@ENDIF\n";
    }

    const auto out_bin = work_dir / "out.bin";
    const auto res = run_process(exe + " -c \"" + app_path.string() + "\" \"" + out_bin.string() + "\" --arch=x86_64");
    assert(res.exit_code == 0);
    assert(std::filesystem::exists(out_bin));

    const auto bytes = read_binary_file(out_bin);
    assert(bytes.size() == 4);
    assert(bytes[0] == 0x0F && bytes[1] == 0x05);
    assert(bytes[2] == 0x0F && bytes[3] == 0x05);

    std::filesystem::current_path(prev_cwd);
    std::error_code ec;
    std::filesystem::remove_all(work_dir, ec);
    std::cout << "[PASS] test_database_resolution_and_preprocessor\n";
}

void test_multi_arch_compilation(const std::string& exe, const std::filesystem::path& tmpl_dir) {
    const auto work_dir = std::filesystem::temp_directory_path() / "test_cli_multi_arch";
    std::filesystem::create_directories(work_dir);

    const auto prev_cwd = std::filesystem::current_path();
    std::filesystem::current_path(work_dir);

    const auto yaml_arm = tmpl_dir / "neo_asm_aarch64.yaml";
    const auto neodb_arm = work_dir / "aarch64.neodb";
    assert(run_process(exe + " -t \"" + yaml_arm.string() + "\" \"" + neodb_arm.string() + "\"").exit_code == 0);
    assert(run_process(exe + " -idb \"" + neodb_arm.string() + "\"").exit_code == 0);

    const auto arm_neoa = work_dir / "arm.neoa";
    {
        std::ofstream out(arm_neoa);
        out << "@IF __arch__ == \"aarch64\"\norigin(SVC 0)\n@ENDIF\n";
    }
    const auto arm_bin = work_dir / "arm.bin";
    assert(run_process(exe + " -c \"" + arm_neoa.string() + "\" \"" + arm_bin.string() + "\" --arch=aarch64").exit_code == 0);
    const auto arm_bytes = read_binary_file(arm_bin);
    assert(arm_bytes.size() == 4);
    assert(arm_bytes[0] == 0xD4 && arm_bytes[1] == 0x00 && arm_bytes[2] == 0x00 && arm_bytes[3] == 0x01);

    const auto yaml_rv = tmpl_dir / "neo_asm_riscv64.yaml";
    const auto neodb_rv = work_dir / "riscv64.neodb";
    assert(run_process(exe + " -t \"" + yaml_rv.string() + "\" \"" + neodb_rv.string() + "\"").exit_code == 0);
    assert(run_process(exe + " -idb \"" + neodb_rv.string() + "\"").exit_code == 0);

    const auto rv_neoa = work_dir / "rv.neoa";
    {
        std::ofstream out(rv_neoa);
        out << "@IF __arch__ == \"riscv64\"\norigin(ECALL)\n@ENDIF\n";
    }
    const auto rv_bin = work_dir / "rv.bin";
    assert(run_process(exe + " -c \"" + rv_neoa.string() + "\" \"" + rv_bin.string() + "\" --arch=riscv64").exit_code == 0);
    const auto rv_bytes = read_binary_file(rv_bin);
    assert(rv_bytes.size() == 4);
    assert(rv_bytes[0] == 0x00 && rv_bytes[1] == 0x00 && rv_bytes[2] == 0x00 && rv_bytes[3] == 0x73);

    std::filesystem::current_path(prev_cwd);
    std::error_code ec;
    std::filesystem::remove_all(work_dir, ec);
    std::cout << "[PASS] test_multi_arch_compilation\n";
}

void test_roundtrip_fidelity(const std::string& exe, const std::filesystem::path& tmpl_dir) {
    const auto yaml_in = tmpl_dir / "neo_asm_x86_64.yaml";
    const auto neodb_1 = std::filesystem::temp_directory_path() / "cli_rt_step1.neodb";
    const auto yaml_2 = std::filesystem::temp_directory_path() / "cli_rt_step2.yaml";
    const auto neodb_3 = std::filesystem::temp_directory_path() / "cli_rt_step3.neodb";

    assert(run_process(exe + " -t \"" + yaml_in.string() + "\" \"" + neodb_1.string() + "\"").exit_code == 0);
    assert(run_process(exe + " -t \"" + neodb_1.string() + "\" \"" + yaml_2.string() + "\"").exit_code == 0);
    assert(run_process(exe + " -t \"" + yaml_2.string() + "\" \"" + neodb_3.string() + "\"").exit_code == 0);

    const auto b1 = read_binary_file(neodb_1);
    const auto b3 = read_binary_file(neodb_3);
    assert(!b1.empty() && b1.size() == b3.size());
    assert(std::memcmp(b1.data(), b3.data(), b1.size()) == 0);

    std::error_code ec;
    std::filesystem::remove(neodb_1, ec);
    std::filesystem::remove(yaml_2, ec);
    std::filesystem::remove(neodb_3, ec);
    std::cout << "[PASS] test_roundtrip_fidelity\n";
}

} // namespace

void run_cli_workflow_tests(const std::string& exe, const std::filesystem::path& tmpl_dir) {
    test_database_installation(exe, tmpl_dir);
    test_database_resolution_and_preprocessor(exe, tmpl_dir);
    test_multi_arch_compilation(exe, tmpl_dir);
    test_roundtrip_fidelity(exe, tmpl_dir);
}

} // namespace neo::test
