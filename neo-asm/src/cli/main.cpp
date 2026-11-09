/**
 * @file main.cpp
 * @brief Command-line application entry point for neo-asm.
 */

#include "cli_commands.hpp"

#include <CLI/CLI.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#if defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

/**
 * @brief Locates the directory containing the currently running executable.
 */
std::filesystem::path locate_executable_directory(const char* argv0) {
#if defined(__linux__)
    std::error_code linux_ec;
    const auto p = std::filesystem::canonical("/proc/self/exe", linux_ec);
    if (!linux_ec) {
        return p.parent_path();
    }
#elif defined(_WIN32)
    std::vector<wchar_t> buffer(1024);
    const DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len > 0) {
        return std::filesystem::path(buffer.data()).parent_path();
    }
#endif
    std::error_code fallback_ec;
    return std::filesystem::weakly_canonical(std::filesystem::path(argv0), fallback_ec).parent_path();
}

/**
 * @brief Normalizes arguments to rewrite '-idb' to '--idb' for CLI11.
 */
std::vector<std::string> normalize_arguments(int argc, char** argv) {
    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc));

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-idb") {
            args.push_back("--idb");
        } else if (arg.rfind("-idb=", 0) == 0) {
            args.push_back("--idb=" + arg.substr(5));
        } else {
            args.push_back(arg);
        }
    }
    return args;
}

/**
 * @brief Parsed command-line options.
 */
struct CliOptions {
    bool translate{false};
    bool compile{false};
    std::string install_db;
    std::string arch;
    std::string custom_db;
    std::string input;
    std::string output;
};

/**
 * @brief Registers command line options and positional arguments in CLI11.
 */
void configure_cli_app(CLI::App& app, CliOptions& opt) {
    app.description("neo-asm: Unified Cross-Architecture Assembly and Opcode Database Toolchain");

    app.add_flag("-t,--translate", opt.translate,
                 "Bidirectional translation between YAML and .neodb formats");
    app.add_flag("-c,--compile", opt.compile,
                 "Compile Neo-ASM assembly source (.neoa) to flat binary (.bin)");
    app.add_option("-i,--install-database,--idb", opt.install_db,
                   "Install a .neodb file into the ./database/ directory");
    app.add_option("--arch", opt.arch,
                   "Target architecture for compilation (e.g. x86_64, aarch64, riscv64)");
    app.add_option("--db", opt.custom_db,
                   "Explicit override path to target architecture .neodb file");
    app.add_option("input", opt.input,
                   "Input specification, database, or assembly source file");
    app.add_option("output", opt.output,
                   "Output destination file");
}

/**
 * @brief Dispatches validated command line arguments to corresponding command routines.
 */
int dispatch_command(
    const CliOptions& opt,
    const std::filesystem::path& exe_dir,
    const CLI::App& app)
{
    if (!opt.install_db.empty()) {
        return neo::cli::execute_install_database(opt.install_db, exe_dir);
    }

    if (opt.translate) {
        return neo::cli::execute_translation(opt.input, opt.output);
    }

    if (opt.compile) {
        return neo::cli::execute_compilation(opt.input, opt.output, opt.arch, opt.custom_db, exe_dir);
    }

    std::cerr << "Error: No command specified. Use --translate, --compile, or --install-database.\n\n"
              << app.help() << "\n";
    return 1;
}

/**
 * @brief Top-level CLI argument parsing and error routing.
 */
int run_cli(int argc, char** argv) {
    CLI::App app;
    CliOptions opt;
    configure_cli_app(app, opt);

    // Requirement: No arguments must display usage help on stderr and exit with code 1
    if (argc <= 1) {
        std::cerr << app.help() << "\n";
        return 1;
    }

    const auto norm_args = normalize_arguments(argc, argv);
    std::vector<const char*> c_args;
    c_args.reserve(norm_args.size() + 1);
    c_args.push_back(argv[0]);
    for (const auto& a : norm_args) {
        c_args.push_back(a.c_str());
    }

    try {
        app.parse(static_cast<int>(c_args.size()), c_args.data());
    } catch (const CLI::CallForHelp& e) {
        return app.exit(e);
    } catch (const CLI::ParseError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    const auto exe_dir = locate_executable_directory(argv[0]);
    return dispatch_command(opt, exe_dir, app);
}

} // namespace

int main(int argc, char** argv) {
    try {
        return run_cli(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Error: Fatal unhandled exception occurred.\n";
        return 1;
    }
}
