/**
 * @file compiler_database_locator.cpp
 * @brief Implementation of architecture database locator and automatic builder.
 */

#include "compiler_database_locator.hpp"
#include "neo/translator.hpp"

#include <algorithm>
#include <vector>

namespace neo::detail {

namespace {

std::string normalize_arch_name(std::string_view arch) {
    std::string s(arch);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (s == "x86-64") return "x86_64";
    if (s == "arm64") return "aarch64";
    if (s == "riscv") return "riscv64";
    return s;
}

std::vector<std::filesystem::path> get_database_candidates(std::string_view arch_norm) {
    const std::string filename = std::string(arch_norm) + ".neodb";
    const std::string test_filename = "test_" + filename;
    return {
        std::filesystem::current_path() / "database" / filename,
        std::filesystem::current_path() / filename,
        std::filesystem::temp_directory_path() / test_filename,
        std::filesystem::temp_directory_path() / filename,
    };
}

std::vector<std::filesystem::path> get_yaml_template_candidates(std::string_view arch_norm) {
    const std::string filename = "neo_asm_" + std::string(arch_norm) + ".yaml";
    return {
        std::filesystem::path("/home/tvdquang/Code/ai-driven/templates") / filename,
        std::filesystem::current_path() / "../../ai-driven/templates" / filename,
        std::filesystem::current_path() / "../../../ai-driven/templates" / filename,
        std::filesystem::current_path() / "ai-driven/templates" / filename,
        std::filesystem::current_path() / "templates" / filename,
    };
}

bool try_build_from_template(std::string_view arch_norm, const std::filesystem::path& out_db) {
    for (const auto& yaml_candidate : get_yaml_template_candidates(arch_norm)) {
        if (std::filesystem::exists(yaml_candidate)) {
            if (NeoTranslator::yaml_to_neodb(yaml_candidate, out_db)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

std::unique_ptr<NeoDatabase> locate_database(
    std::string_view arch,
    const std::filesystem::path& custom_db_path) {
    auto db = std::make_unique<NeoDatabase>();

    // 1. Explicit path given
    if (!custom_db_path.empty() && std::filesystem::exists(custom_db_path)) {
        if (db->load_file(custom_db_path)) {
            return db;
        }
        return nullptr;
    }

    const std::string arch_norm = normalize_arch_name(arch);

    // 2. Search predefined .neodb file candidates
    for (const auto& path : get_database_candidates(arch_norm)) {
        if (std::filesystem::exists(path) && db->load_file(path)) {
            return db;
        }
    }

    // 3. Fallback: compile YAML template to temporary .neodb on demand
    const auto temp_db = std::filesystem::temp_directory_path() / ("test_" + arch_norm + ".neodb");
    if (try_build_from_template(arch_norm, temp_db) && db->load_file(temp_db)) {
        return db;
    }

    return nullptr;
}

} // namespace neo::detail
