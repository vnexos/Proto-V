/**
 * @file test_mmap_unit.cpp
 * @brief Unit tests exercising RAII memory mapping, move semantics, and error handling.
 */

#include <neo/mmap.hpp>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

/**
 * @brief Creates a temporary binary file with specified size and pattern.
 * @param path Destination filesystem path.
 * @param size Number of bytes to write.
 */
void create_temp_file(const std::filesystem::path& path, size_t size) {
    std::ofstream out(path, std::ios::binary);
    std::vector<uint8_t> buffer(size);
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = static_cast<uint8_t>((i * 7 + 13) & 0xFF);
    }
    if (size > 0) {
        out.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(size));
    }
}

/**
 * @brief Tests opening an existing valid binary file and reading its contents.
 */
void test_open_valid_file() {
    const auto temp_path = std::filesystem::temp_directory_path() / "neo_test_valid.bin";
    const size_t file_size = 256;
    create_temp_file(temp_path, file_size);

    neo::MemoryMappedFile mmap_file;
    assert(!mmap_file.is_open());
    assert(mmap_file.data() == nullptr);
    assert(mmap_file.size() == 0);

    const bool opened = mmap_file.open(temp_path);
    assert(opened);
    assert(mmap_file.is_open());
    assert(mmap_file.size() == file_size);
    assert(mmap_file.data() != nullptr);

    for (size_t i = 0; i < file_size; ++i) {
        const uint8_t expected = static_cast<uint8_t>((i * 7 + 13) & 0xFF);
        assert(mmap_file.data()[i] == expected);
    }

    auto span = mmap_file.bytes();
    assert(span.size() == file_size);
    assert(span[10] == static_cast<uint8_t>((10 * 7 + 13) & 0xFF));

    mmap_file.close();
    assert(!mmap_file.is_open());
    assert(mmap_file.data() == nullptr);
    assert(mmap_file.size() == 0);

    std::filesystem::remove(temp_path);
    std::cout << "[PASS] test_open_valid_file\n";
}

/**
 * @brief Tests move construction and move assignment semantics without leaks.
 */
void test_move_semantics() {
    const auto temp_path = std::filesystem::temp_directory_path() / "neo_test_move.bin";
    const size_t file_size = 128;
    create_temp_file(temp_path, file_size);

    neo::MemoryMappedFile file1;
    assert(file1.open(temp_path));

    // Test move construction
    neo::MemoryMappedFile file2(std::move(file1));
    assert(!file1.is_open());
    assert(file1.data() == nullptr);
    assert(file1.size() == 0);
    assert(file2.is_open());
    assert(file2.size() == file_size);
    assert(file2.data() != nullptr);

    // Test move assignment
    neo::MemoryMappedFile file3;
    file3 = std::move(file2);
    assert(!file2.is_open());
    assert(file2.data() == nullptr);
    assert(file2.size() == 0);
    assert(file3.is_open());
    assert(file3.size() == file_size);

    // Test self move assignment
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
#endif
    auto& self_ref = file3;
    file3 = std::move(self_ref);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    assert(file3.is_open());
    assert(file3.size() == file_size);

    std::filesystem::remove(temp_path);
    std::cout << "[PASS] test_move_semantics\n";
}

/**
 * @brief Tests opening a non-existent file returns false gracefully without exception.
 */
void test_non_existent_file() {
    const auto non_existent = std::filesystem::temp_directory_path() / "neo_non_existent_12345.bin";
    std::filesystem::remove(non_existent);

    neo::MemoryMappedFile mmap_file;
    const bool opened = mmap_file.open(non_existent);
    assert(!opened);
    assert(!mmap_file.is_open());
    assert(mmap_file.data() == nullptr);
    assert(mmap_file.size() == 0);

    std::cout << "[PASS] test_non_existent_file\n";
}

/**
 * @brief Tests opening a zero-length empty file returns false without throwing.
 */
void test_empty_file() {
    const auto temp_path = std::filesystem::temp_directory_path() / "neo_test_empty.bin";
    create_temp_file(temp_path, 0);

    neo::MemoryMappedFile mmap_file;
    const bool opened = mmap_file.open(temp_path);
    assert(!opened);
    assert(!mmap_file.is_open());
    assert(mmap_file.data() == nullptr);
    assert(mmap_file.size() == 0);

    std::filesystem::remove(temp_path);
    std::cout << "[PASS] test_empty_file\n";
}

/**
 * @brief Tests re-opening another file on an active instance and idempotent closure.
 */
void test_reopen_and_close() {
    const auto path1 = std::filesystem::temp_directory_path() / "neo_reopen_1.bin";
    const auto path2 = std::filesystem::temp_directory_path() / "neo_reopen_2.bin";
    create_temp_file(path1, 64);
    create_temp_file(path2, 192);

    neo::MemoryMappedFile mmap_file;
    assert(mmap_file.open(path1));
    assert(mmap_file.size() == 64);

    // Reopen path2 without calling close() first
    assert(mmap_file.open(path2.string()));
    assert(mmap_file.size() == 192);

    // Idempotent close
    mmap_file.close();
    mmap_file.close();
    assert(!mmap_file.is_open());

    std::filesystem::remove(path1);
    std::filesystem::remove(path2);
    std::cout << "[PASS] test_reopen_and_close\n";
}

} // namespace

int main() {
    test_open_valid_file();
    test_move_semantics();
    test_non_existent_file();
    test_empty_file();
    test_reopen_and_close();
    std::cout << "All test_mmap_unit tests passed successfully.\n";
    return 0;
}
