/**
 * @file mmap.hpp
 * @brief Cross-platform RAII memory-mapped file abstraction for Linux and Windows.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace neo {

/**
 * @brief Cross-platform RAII wrapper for zero-copy read-only memory-mapped files.
 *
 * Implements safe, zero-copy, read-only file mapping using Linux POSIX mmap
 * and Windows Win32 MapViewOfFile. Enforces strict cleanup order in destructor
 * and supports move semantics with noexcept guarantees.
 */
class MemoryMappedFile {
public:
    /**
     * @brief Constructs an unmapped MemoryMappedFile instance.
     */
    MemoryMappedFile() noexcept = default;

    /**
     * @brief Destructor safely unmapping memory and closing OS handles.
     */
    ~MemoryMappedFile();

    // Non-copyable to prevent double-unmapping or handle duplication
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

    /**
     * @brief Move constructor transferring file mapping ownership.
     * @param other Rvalue reference to source MemoryMappedFile.
     */
    MemoryMappedFile(MemoryMappedFile&& other) noexcept;

    /**
     * @brief Move assignment operator transferring file mapping ownership.
     * @param other Rvalue reference to source MemoryMappedFile.
     * @return Reference to this instance.
     */
    MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept;

    /**
     * @brief Opens and memory-maps a file in read-only mode from a filesystem path.
     * @param path Target file path.
     * @return True if mapping succeeded, false otherwise.
     */
    bool open(const std::filesystem::path& path);

    /**
     * @brief Opens and memory-maps a file in read-only mode from a string path.
     * @param path Target file path string.
     * @return True if mapping succeeded, false otherwise.
     */
    bool open(const std::string& path);

    /**
     * @brief Safely unmaps the memory view and closes associated OS file handles.
     */
    void close() noexcept;

    /**
     * @brief Returns a pointer to the mapped byte buffer.
     * @return Constant pointer to mapped bytes, or nullptr if not open.
     */
    [[nodiscard]] const uint8_t* data() const noexcept { return data_; }

    /**
     * @brief Returns the size of the mapped file in bytes.
     * @return File size in bytes, or 0 if not open.
     */
    [[nodiscard]] size_t size() const noexcept { return size_; }

    /**
     * @brief Checks if a file is currently successfully mapped.
     * @return True if a valid mapping exists, false otherwise.
     */
    [[nodiscard]] bool is_open() const noexcept { return data_ != nullptr; }

    /**
     * @brief Returns a contiguous span view over mapped bytes.
     * @return Read-only span of bytes.
     */
    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept {
        return {data_, size_};
    }

private:
    const uint8_t* data_{nullptr};
    size_t size_{0};

#if defined(_WIN32)
    void* file_handle_{reinterpret_cast<void*>(-1)};
    void* mapping_handle_{nullptr};
#else
    int fd_{-1};
#endif
};

} // namespace neo
