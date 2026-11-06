/**
 * @file database.cpp
 * @brief Zero-copy opcode database loader and frozen hash query engine implementation.
 */

#include <neo/database.hpp>
#include <cstring>

namespace neo {

namespace {

DatabaseError check_header_integrity(const NeoDbHeader& hdr, std::span<const uint8_t> buf) noexcept {
    if (std::memcmp(hdr.magic, "NEODB\0\0\0", 8) != 0) return DatabaseError::InvalidMagic;
    if (hdr.format_version != 0x00010000) return DatabaseError::InvalidVersion;
    if (hdr.header_crc32 != calcCRC32(buf.subspan(0, 60))) return DatabaseError::CrcMismatch;
    if ((hdr.strtab_offset % 8 != 0) || (hdr.index_offset % 8 != 0) || (hdr.template_offset % 8 != 0)) {
        return DatabaseError::UnalignedSection;
    }
    return DatabaseError::None;
}

DatabaseError check_section_bounds(const NeoDbHeader& hdr, size_t file_size) noexcept {
    if (hdr.strtab_offset < sizeof(NeoDbHeader) || hdr.index_offset < sizeof(NeoDbHeader) ||
        hdr.template_offset < sizeof(NeoDbHeader)) return DatabaseError::OutOfBoundsSection;
    if (hdr.strtab_offset > file_size || hdr.strtab_size > file_size - hdr.strtab_offset) {
        return DatabaseError::OutOfBoundsSection;
    }
    const uint64_t idx_bytes = static_cast<uint64_t>(hdr.index_bucket_count) * sizeof(NeoIndexEntry);
    if (hdr.index_bucket_count == 0 || hdr.index_offset > file_size || idx_bytes > file_size - hdr.index_offset) {
        return DatabaseError::OutOfBoundsSection;
    }
    if (hdr.template_offset > file_size || hdr.template_size > file_size - hdr.template_offset ||
        hdr.template_size % sizeof(NeoInstructionTemplate) != 0) {
        return DatabaseError::OutOfBoundsSection;
    }
    return DatabaseError::None;
}

} // namespace

std::string_view to_string(DatabaseError error) noexcept {
    switch (error) {
        case DatabaseError::None: return "No error";
        case DatabaseError::FileOpenFailed: return "Failed to open database file";
        case DatabaseError::FileTooSmall: return "File size is smaller than header size";
        case DatabaseError::InvalidMagic: return "Invalid NEODB magic bytes in header";
        case DatabaseError::InvalidVersion: return "Unsupported or invalid database format version";
        case DatabaseError::CrcMismatch: return "Header CRC32 checksum mismatch";
        case DatabaseError::UnalignedSection: return "Section offset is not 8-byte aligned";
        case DatabaseError::OutOfBoundsSection: return "Section bounds exceed file size or invalid layout";
    }
    return "Unknown error";
}

const NeoIndexEntry* find_instruction(
    std::string_view mnemonic,
    const NeoIndexEntry* buckets,
    uint32_t bucket_count,
    const char* strtab,
    uint32_t strtab_size) noexcept {
    if (!buckets || bucket_count == 0 || !strtab || mnemonic.empty()) {
        return nullptr;
    }

    const uint32_t target_hash = fnv1a_32(mnemonic);
    uint32_t idx = target_hash % bucket_count;

    for (uint32_t probe = 0; probe < bucket_count; ++probe) {
        const auto& entry = buckets[idx];
        if (entry.name_hash == 0 || entry.name_str_offset >= strtab_size) {
            return nullptr;
        }

        if (entry.name_hash == target_hash && entry.name_str_offset + mnemonic.size() < strtab_size) {
            const char* stored = strtab + entry.name_str_offset;
            if (stored[mnemonic.size()] == '\0' &&
                std::memcmp(stored, mnemonic.data(), mnemonic.size()) == 0) {
                return &entry;
            }
        }
        idx = (idx + 1) % bucket_count;
    }
    return nullptr;
}

NeoDatabase::~NeoDatabase() { close(); }

NeoDatabase::NeoDatabase(NeoDatabase&& other) noexcept { *this = std::move(other); }

NeoDatabase& NeoDatabase::operator=(NeoDatabase&& other) noexcept {
    if (this != &other) {
        close();
        mmap_ = std::move(other.mmap_);
        buffer_ = other.buffer_;
        header_ = other.header_;
        strtab_ = other.strtab_;
        index_entries_ = other.index_entries_;
        template_data_ = other.template_data_;
        last_error_ = other.last_error_;

        other.buffer_ = {};
        other.header_ = nullptr;
        other.strtab_ = nullptr;
        other.index_entries_ = {};
        other.template_data_ = nullptr;
        other.last_error_ = DatabaseError::None;
    }
    return *this;
}

bool NeoDatabase::load_file(const std::filesystem::path& path) {
    close();
    if (!mmap_.open(path)) {
        last_error_ = DatabaseError::FileOpenFailed;
        return false;
    }
    if (!validate_and_map(mmap_.bytes())) {
        mmap_.close();
        return false;
    }
    return true;
}

bool NeoDatabase::load_buffer(std::span<const uint8_t> buffer) noexcept {
    close();
    return validate_and_map(buffer);
}

void NeoDatabase::close() noexcept {
    header_ = nullptr;
    strtab_ = nullptr;
    index_entries_ = {};
    template_data_ = nullptr;
    buffer_ = {};
    mmap_.close();
    last_error_ = DatabaseError::None;
}

const NeoIndexEntry* NeoDatabase::find_entry(std::string_view name) const noexcept {
    if (!header_ || !strtab_ || index_entries_.empty()) return nullptr;
    return find_instruction(name, index_entries_.data(), header_->index_bucket_count,
                            strtab_, header_->strtab_size);
}

std::span<const NeoInstructionTemplate> NeoDatabase::get_templates(
    const NeoIndexEntry& entry) const noexcept {
    if (!header_ || !template_data_ || entry.template_count == 0) {
        return {};
    }
    const uint64_t bytes_needed = static_cast<uint64_t>(entry.template_count) * sizeof(NeoInstructionTemplate);
    // Subtraction-based bound check prevents unsigned integer overflow if template_offset + bytes_needed wraps
    if (entry.template_offset > header_->template_size ||
        bytes_needed > header_->template_size - entry.template_offset) {
        return {};
    }
    const auto* ptr = reinterpret_cast<const NeoInstructionTemplate*>(template_data_ + entry.template_offset);
    return {ptr, entry.template_count};
}

const char* NeoDatabase::get_string(uint32_t offset) const noexcept {
    if (!header_ || !strtab_ || offset >= header_->strtab_size) return nullptr;
    const void* term = std::memchr(strtab_ + offset, '\0', header_->strtab_size - offset);
    return term ? strtab_ + offset : nullptr;
}

const NeoDbHeader& NeoDatabase::header() const noexcept {
    static constexpr NeoDbHeader s_empty{};
    return header_ ? *header_ : s_empty;
}

bool NeoDatabase::validate_and_map(std::span<const uint8_t> buffer) noexcept {
    if (buffer.size() < sizeof(NeoDbHeader)) {
        last_error_ = DatabaseError::FileTooSmall;
        return false;
    }
    const auto* hdr = reinterpret_cast<const NeoDbHeader*>(buffer.data());
    const DatabaseError hdr_err = check_header_integrity(*hdr, buffer);
    if (hdr_err != DatabaseError::None) {
        last_error_ = hdr_err;
        return false;
    }
    const DatabaseError bounds_err = check_section_bounds(*hdr, buffer.size());
    if (bounds_err != DatabaseError::None) {
        last_error_ = bounds_err;
        return false;
    }
    buffer_ = buffer;
    header_ = hdr;
    strtab_ = reinterpret_cast<const char*>(buffer.data() + hdr->strtab_offset);
    index_entries_ = {reinterpret_cast<const NeoIndexEntry*>(buffer.data() + hdr->index_offset),
                      hdr->index_bucket_count};
    template_data_ = buffer.data() + hdr->template_offset;
    last_error_ = DatabaseError::None;
    return true;
}

} // namespace neo
