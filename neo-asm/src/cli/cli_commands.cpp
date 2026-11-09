/**
 * @file cli_commands.cpp
 * @brief Implementation of command execution routines for the neo-asm CLI driver.
 */

#include "cli_commands.hpp"

#include <neo/compiler.hpp>
#include <neo/database.hpp>
#include <neo/translator.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>

namespace neo::cli {

namespace {

std::string to_lower_string(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

bool copy_database_to_dir(const std::filesystem::path& src, const std::filesystem::path& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) return false;

    const auto dest = dir / src.filename();
    std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
}

} // namespace

std::filesystem::path resolve_database_path(
    std::string_view arch,
    const std::filesystem::path& custom_db,
    const std::filesystem::path& exe_dir)
{
    if (!custom_db.empty()) return custom_db;

    const std::string filename = std::string(arch) + ".neodb";

    // 1. Check relative to executable directory: <exe_dir>/database/<arch>.neodb
    if (!exe_dir.empty()) {
        const auto exe_db = exe_dir / "database" / filename;
        if (std::filesystem::exists(exe_db)) return exe_db;
    }

    // 2. Check relative to current working directory: ./database/<arch>.neodb
    const auto cwd_db = std::filesystem::current_path() / "database" / filename;
    if (std::filesystem::exists(cwd_db)) return cwd_db;

    // 3. Check current working directory: ./<arch>.neodb
    const auto local_db = std::filesystem::current_path() / filename;
    if (std::filesystem::exists(local_db)) return local_db;

    return {};
}

int execute_install_database(const std::filesystem::path& db_path, const std::filesystem::path& exe_dir) {
    if (db_path.empty()) {
        std::cerr << "Error: Database installation requires a file path.\n";
        return 1;
    }
    if (!std::filesystem::exists(db_path)) {
        std::cerr << "Error: Database file does not exist: " << db_path.string() << "\n";
        return 1;
    }

    // Validate header magic, version, and CRC32 integrity before installing
    NeoDatabase db;
    if (!db.load_file(db_path)) {
        std::cerr << "Error: Invalid database file: " << to_string(db.last_error())
                  << " (" << db_path.string() << ")\n";
        return 1;
    }

    const auto cwd_db_dir = std::filesystem::current_path() / "database";
    if (!copy_database_to_dir(db_path, cwd_db_dir)) {
        std::cerr << "Error: Failed to copy database to " << cwd_db_dir.string() << "\n";
        return 1;
    }

    // Also mirror to executable directory if distinct
    if (!exe_dir.empty() && (exe_dir / "database") != cwd_db_dir) {
        copy_database_to_dir(db_path, exe_dir / "database");
    }

    std::cout << "Successfully installed database to "
              << (cwd_db_dir / db_path.filename()).string() << "\n";
    return 0;
}

int execute_translation(const std::filesystem::path& in_path, const std::filesystem::path& out_path) {
    if (in_path.empty() || out_path.empty()) {
        std::cerr << "Error: Translation requires both <input> and <output> file paths.\n";
        return 1;
    }
    if (!std::filesystem::exists(in_path)) {
        std::cerr << "Error: Input file does not exist: " << in_path.string() << "\n";
        return 1;
    }

    const std::string ext = to_lower_string(in_path.extension().string());

    if (ext == ".yaml" || ext == ".yml") {
        if (!NeoTranslator::yaml_to_neodb(in_path, out_path)) {
            std::cerr << "Error: Failed to translate YAML to .neodb: " << in_path.string() << "\n";
            return 1;
        }
        return 0;
    }

    if (ext == ".neodb") {
        // Validate database file integrity prior to decompilation
        NeoDatabase db;
        if (!db.load_file(in_path)) {
            std::cerr << "Error: Corrupted database file: " << to_string(db.last_error())
                      << " (" << in_path.string() << ")\n";
            return 1;
        }

        if (!NeoTranslator::neodb_to_yaml(in_path, out_path)) {
            std::cerr << "Error: Failed to decompile .neodb to YAML: " << in_path.string() << "\n";
            return 1;
        }
        return 0;
    }

    std::cerr << "Error: Unsupported input file format: " << in_path.string()
              << " (expected .yaml, .yml, or .neodb)\n";
    return 1;
}

int execute_compilation(
    const std::filesystem::path& in_path,
    const std::filesystem::path& out_path,
    std::string_view arch,
    const std::filesystem::path& custom_db,
    const std::filesystem::path& exe_dir)
{
    if (in_path.empty() || out_path.empty()) {
        std::cerr << "Error: Compilation requires both <input.neoa> and <output.bin> file paths.\n";
        return 1;
    }
    if (arch.empty()) {
        std::cerr << "Error: Compilation requires --arch=<arch> (e.g. x86_64, aarch64, riscv64).\n";
        return 1;
    }
    if (!std::filesystem::exists(in_path)) {
        std::cerr << "Error: Input assembly file does not exist: " << in_path.string() << "\n";
        return 1;
    }
    if (!custom_db.empty() && !std::filesystem::exists(custom_db)) {
        std::cerr << "Error: Specified database file does not exist: " << custom_db.string() << "\n";
        return 1;
    }

    const auto resolved_db = resolve_database_path(arch, custom_db, exe_dir);
    if (!NeoCompiler::compile_file(in_path, out_path, arch, resolved_db)) {
        return 1;
    }

    return 0;
}

} // namespace neo::cli
