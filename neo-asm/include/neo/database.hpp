/**
 * @file database.hpp
 * @brief Zero-copy read-only opcode database (.neodb) loader and frozen hash query engine.
 */

#pragma once

#include <neo/common.hpp>
#include <neo/mmap.hpp>
#include <neo/schema.hpp>

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

namespace neo {

/**
 * @brief Categorizes database load and validation failure reasons.
 */
enum class DatabaseError : uint8_t {
    None = 0,           ///< No error; operation succeeded
    FileOpenFailed,     ///< File could not be opened or mapped
    FileTooSmall,       ///< Buffer/file size is smaller than 64-byte header
    InvalidMagic,       ///< Header magic bytes mismatch ("NEODB\0\0\0")
    InvalidVersion,     ///< Header version unsupported (not 0x00010000)
    CrcMismatch,        ///< Header CRC32 checksum mismatch
    UnalignedSection,   ///< Section offset is not 8-byte aligned
    OutOfBoundsSection  ///< Section bounds exceed file bounds or invalid layout
};

/**
 * @brief Converts a DatabaseError code to a human-readable diagnostic message.
 * @param error DatabaseError code to convert.
 * @return Descriptive string view for diagnostics and error reporting.
 */
[[nodiscard]] std::string_view to_string(DatabaseError error) noexcept;

/**
 * @brief Searches for a mnemonic in a frozen hash index bucket array.
 *
 * Implements O(1) query lookups using 32-bit FNV-1a hashing with open addressing
 * and linear probing over the .idx section. Halts immediately upon encountering
 * an empty bucket or matching the mnemonic in the string table.
 *
 * @param mnemonic Opcode mnemonic name to search for.
 * @param buckets Pointer to contiguous array of NeoIndexEntry records.
 * @param bucket_count Total number of buckets in index array.
 * @param strtab Pointer to base of null-terminated string table.
 * @param strtab_size Total size of string table in bytes.
 * @return Pointer to matching NeoIndexEntry, or nullptr if not found.
 */
[[nodiscard]] const NeoIndexEntry* find_instruction(
    std::string_view mnemonic,
    const NeoIndexEntry* buckets,
    uint32_t bucket_count,
    const char* strtab,
    uint32_t strtab_size) noexcept;

/**
 * @brief Zero-copy memory-mapped .neodb database reader.
 *
 * Provides safe, zero-copy, read-only access to packed binary .neodb files.
 * Validates 64-byte header integrity (magic, version, CRC32, 8-byte alignment)
 * and section bounds without any dynamic heap allocations.
 */
class NeoDatabase {
public:
    NeoDatabase() noexcept = default;
    ~NeoDatabase();

    // Non-copyable to prevent duplicate mapping handles
    NeoDatabase(const NeoDatabase&) = delete;
    NeoDatabase& operator=(const NeoDatabase&) = delete;

    // Move-constructible and move-assignable
    NeoDatabase(NeoDatabase&& other) noexcept;
    NeoDatabase& operator=(NeoDatabase&& other) noexcept;

    /**
     * @brief Opens and validates a .neodb file via zero-copy memory mapping.
     * @param path Path to the .neodb file.
     * @return True if file is valid and mapped, false on validation failure.
     */
    bool load_file(const std::filesystem::path& path);

    /**
     * @brief Validates and maps an in-memory byte buffer directly.
     * @param buffer Read-only span of bytes representing a .neodb image.
     * @return True if buffer is valid, false otherwise.
     */
    bool load_buffer(std::span<const uint8_t> buffer) noexcept;

    /**
     * @brief Closes the database and unmaps any memory-mapped file.
     */
    void close() noexcept;

    /**
     * @brief Checks if a database is currently loaded and valid.
     * @return True if loaded, false otherwise.
     */
    [[nodiscard]] bool is_loaded() const noexcept { return header_ != nullptr; }

    /**
     * @brief Returns the error code from the most recent load operation.
     * @return DatabaseError code.
     */
    [[nodiscard]] DatabaseError last_error() const noexcept { return last_error_; }

    /**
     * @brief Returns a descriptive human-readable message for the last error.
     * @return Diagnostic error string view.
     */
    [[nodiscard]] std::string_view last_error_message() const noexcept { return to_string(last_error_); }

    /**
     * @brief Queries an instruction by mnemonic name in O(1) time.
     * @param name Opcode mnemonic name or origin(...) command.
     * @return Pointer to NeoIndexEntry residing in mapped memory, or nullptr.
     */
    [[nodiscard]] const NeoIndexEntry* find_entry(std::string_view name) const noexcept;

    /**
     * @brief Returns a span view over instruction templates for an index entry.
     * @param entry Target NeoIndexEntry previously found in database.
     * @return Span of NeoInstructionTemplate records residing in mapped memory.
     */
    [[nodiscard]] std::span<const NeoInstructionTemplate> get_templates(
        const NeoIndexEntry& entry) const noexcept;

    /**
     * @brief Retrieves a null-terminated string from string table by offset.
     * @param offset Byte offset relative to start of string table.
     * @return Pointer to string in mapped memory, or nullptr if out of bounds.
     */
    [[nodiscard]] const char* get_string(uint32_t offset) const noexcept;

    /**
     * @brief Returns the 64-byte database header.
     * @return Reference to NeoDbHeader in mapped memory, or static empty header.
     */
    [[nodiscard]] const NeoDbHeader& header() const noexcept;

    /**
     * @brief Returns the raw memory-mapped byte buffer.
     * @return Span of mapped bytes.
     */
    [[nodiscard]] std::span<const uint8_t> raw_bytes() const noexcept { return buffer_; }

private:
    [[nodiscard]] bool validate_and_map(std::span<const uint8_t> buffer) noexcept;

    MemoryMappedFile mmap_{};
    std::span<const uint8_t> buffer_{};
    const NeoDbHeader* header_{nullptr};
    const char* strtab_{nullptr};
    std::span<const NeoIndexEntry> index_entries_{};
    const uint8_t* template_data_{nullptr};
    DatabaseError last_error_{DatabaseError::None};
};

} // namespace neo
