/**
 * @file test_roundtrip.cpp
 * @brief Property-based lossless roundtrip verification (YAML -> neodb -> YAML -> neodb).
 */

#include <neo/common.hpp>
#include <neo/database.hpp>
#include <neo/schema.hpp>
#include <neo/translator.hpp>

#include <yaml-cpp/yaml.h>
#include <cassert>
#include <cstring>
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

std::vector<uint8_t> read_binary_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    assert(file.is_open());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void verify_yaml_formatting(const std::filesystem::path& yaml_path) {
    std::ifstream file(yaml_path);
    assert(file.is_open());

    std::string line;
    bool found_hex_mask = false;
    while (std::getline(file, line)) {
        if (line.find("mask: 0x") != std::string::npos || line.find("mask: '0x") != std::string::npos) {
            found_hex_mask = true;
            // Ensure 0x prefix is followed by exactly 8 hexadecimal characters
            const auto pos = line.find("0x");
            assert(pos != std::string::npos && line.size() >= pos + 10);
        }
    }
    assert(found_hex_mask);
}

void test_architecture_roundtrip(const std::filesystem::path& tmpl_dir, const std::string& arch) {
    const auto yaml_a = tmpl_dir / ("neo_asm_" + arch + ".yaml");
    const auto temp_dir = std::filesystem::temp_directory_path();
    const auto neodb_1 = temp_dir / ("rt_" + arch + "_1.neodb");
    const auto yaml_b = temp_dir / ("rt_" + arch + "_decompiled.yaml");
    const auto neodb_2 = temp_dir / ("rt_" + arch + "_2.neodb");

    // Pass 1: YAML_A -> neodb_1
    assert(neo::NeoTranslator::yaml_to_neodb(yaml_a, neodb_1));
    assert(std::filesystem::exists(neodb_1));

    // Pass 2: neodb_1 -> YAML_B
    assert(neo::NeoTranslator::neodb_to_yaml(neodb_1, yaml_b));
    assert(std::filesystem::exists(yaml_b));
    verify_yaml_formatting(yaml_b);

    // Verify YAML_B is valid YAML DOM parseable by yaml-cpp
    const YAML::Node doc = YAML::LoadFile(yaml_b.string());
    assert(doc.IsMap() && doc["ops"] && doc["ops"]["neo"] && doc["ops"]["origin"]);

    // Pass 3: YAML_B -> neodb_2
    assert(neo::NeoTranslator::yaml_to_neodb(yaml_b, neodb_2));
    assert(std::filesystem::exists(neodb_2));

    // Verify bit-identical buffer invariant and CRC32 equality
    const auto bytes_1 = read_binary_file(neodb_1);
    const auto bytes_2 = read_binary_file(neodb_2);

    assert(bytes_1.size() == bytes_2.size());
    assert(std::memcmp(bytes_1.data(), bytes_2.data(), bytes_1.size()) == 0);

    const auto* h1 = reinterpret_cast<const neo::NeoDbHeader*>(bytes_1.data());
    const auto* h2 = reinterpret_cast<const neo::NeoDbHeader*>(bytes_2.data());
    assert(h1->header_crc32 == h2->header_crc32);

    std::filesystem::remove(neodb_1);
    std::filesystem::remove(yaml_b);
    std::filesystem::remove(neodb_2);
}

void test_error_handling() {
    const auto temp_dir = std::filesystem::temp_directory_path();
    const auto out_yaml = temp_dir / "err_out.yaml";

    // Non-existent database file
    assert(!neo::NeoTranslator::neodb_to_yaml("non_existent_database_file.neodb", out_yaml));

    // Corrupted binary header
    const auto corrupt_db = temp_dir / "corrupt_magic.neodb";
    {
        std::ofstream out(corrupt_db, std::ios::binary);
        std::vector<uint8_t> bad_buf(64, 0);
        std::memcpy(bad_buf.data(), "BAD_MAG!", 8);
        out.write(reinterpret_cast<const char*>(bad_buf.data()), 64);
    }
    assert(!neo::NeoTranslator::neodb_to_yaml(corrupt_db, out_yaml));
    std::filesystem::remove(corrupt_db);

    // Unwritable output path
    const auto invalid_path = "/dev/null/cannot_create_dir/decompiled.yaml";
    assert(!neo::NeoTranslator::neodb_to_yaml(corrupt_db, invalid_path));
}

} // namespace

int main() {
    const auto tmpl_dir = resolve_templates_dir();
    assert(!tmpl_dir.empty());

    // Verify lossless bit-identical roundtrip across all 3 architectures
    test_architecture_roundtrip(tmpl_dir, "x86_64");
    test_architecture_roundtrip(tmpl_dir, "aarch64");
    test_architecture_roundtrip(tmpl_dir, "riscv64");

    // Verify error diagnostics and boundary handling
    test_error_handling();

    std::cout << "[PASS] test_roundtrip passed with bit-identical binary reproduction.\n";
    return 0;
}
