/**
 * @file compiler.cpp
 * @brief Public interface implementation for NeoCompiler facade.
 */

#include "neo/compiler.hpp"
#include "compiler_database_locator.hpp"
#include "compiler_pipeline.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace neo {

namespace {

thread_local std::vector<std::string> g_last_errors;

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool write_binary_file(const std::filesystem::path& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return out.good();
}

} // namespace

const std::vector<std::string>& NeoCompiler::last_errors() noexcept {
    return g_last_errors;
}

bool NeoCompiler::compile_source(
    std::string_view source,
    const NeoDatabase& db,
    std::vector<uint8_t>& out_bin,
    const std::filesystem::path& current_file) {
    g_last_errors.clear();
    const auto res = detail::CompilerPipeline::run(source, db, out_bin, current_file);
    if (!res.success) {
        g_last_errors = res.errors;
        for (const auto& err : g_last_errors) {
            std::cerr << "[NeoCompiler Error] " << err << "\n";
        }
        return false;
    }
    return true;
}

bool NeoCompiler::compile_file(
    const std::filesystem::path& in_neoa,
    const std::filesystem::path& out_bin,
    const NeoDatabase& db) {
    if (!std::filesystem::exists(in_neoa)) {
        g_last_errors = {"Source file does not exist: " + in_neoa.string()};
        std::cerr << "[NeoCompiler Error] " << g_last_errors.front() << "\n";
        return false;
    }

    const std::string source = read_text_file(in_neoa);
    std::vector<uint8_t> binary_bytes;
    if (!compile_source(source, db, binary_bytes, in_neoa)) {
        return false;
    }

    if (!write_binary_file(out_bin, binary_bytes)) {
        g_last_errors = {"Failed to write output binary to: " + out_bin.string()};
        std::cerr << "[NeoCompiler Error] " << g_last_errors.front() << "\n";
        return false;
    }

    return true;
}

bool NeoCompiler::compile_file(
    const std::filesystem::path& in_neoa,
    const std::filesystem::path& out_bin,
    std::string_view arch,
    const std::filesystem::path& db_path) {
    const auto db = detail::locate_database(arch, db_path);
    if (!db || !db->is_loaded()) {
        g_last_errors = {"Failed to locate or load database for architecture: " + std::string(arch)};
        std::cerr << "[NeoCompiler Error] " << g_last_errors.front() << "\n";
        return false;
    }
    return compile_file(in_neoa, out_bin, *db);
}

} // namespace neo
