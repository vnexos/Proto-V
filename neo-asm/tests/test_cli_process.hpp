/**
 * @file test_cli_process.hpp
 * @brief Subprocess execution harness for CLI integration tests.
 */

#pragma once

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace neo::test {

/**
 * @brief Stores the output and exit status of a subprocess execution.
 */
struct CommandResult {
    int exit_code{-1};
    std::string stdout_output;
    std::string stderr_output;
};

/**
 * @brief Reads the entire contents of a file into a std::string.
 * @param path Path to file.
 * @return File contents as string, or empty string on failure.
 */
inline std::string read_file_to_string(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

/**
 * @brief Executes a command line as a subprocess and captures outputs.
 * @param command Command string to run.
 * @return CommandResult containing exit code, stdout, and stderr.
 */
inline CommandResult run_process(const std::string& command) {
    static std::atomic<uint64_t> counter{0};
    const std::string tag = std::to_string(counter.fetch_add(1));

    const auto out_path = std::filesystem::temp_directory_path() / ("test_cli_stdout_" + tag + ".tmp");
    const auto err_path = std::filesystem::temp_directory_path() / ("test_cli_stderr_" + tag + ".tmp");

    const std::string full_cmd = command + " > \"" + out_path.string() + "\" 2> \"" + err_path.string() + "\"";
    const int raw_status = std::system(full_cmd.c_str());

#if defined(_WIN32)
    const int code = raw_status;
#else
    const int code = WEXITSTATUS(raw_status);
#endif

    CommandResult res{};
    res.exit_code = code;
    res.stdout_output = read_file_to_string(out_path);
    res.stderr_output = read_file_to_string(err_path);

    std::error_code ec;
    std::filesystem::remove(out_path, ec);
    std::filesystem::remove(err_path, ec);
    return res;
}

} // namespace neo::test
