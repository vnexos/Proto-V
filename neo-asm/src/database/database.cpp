/**
 * @file database.cpp
 * @brief Zero-copy opcode database loader and frozen hash query engine implementation.
 */

#include <neo/database.hpp>
#include <cstring>

namespace neo {

namespace {

bool validate_header_integrity(const NeoDbHeader& hdr, std::span<const uint8_t> buf) noexcept {
    if (std::memcmp(hdr.magic, "NEODB\0\0\0", 8) != 0 || hdr.format_version != 0x00010000) {
        return false;
    }
    const uint32_t expected_crc = calcCRC32(buf.subspan(0, 60));
    if (hdr.header_crc32 != expected_crc) {
        return false;
    }
    return (hdr.strtab_offset % 8 == 0) && (hdr.index_offset % 8 == 0) && (hdr.template_offset % 8 == 0);
}

bool validate_section_bounds(const NeoDbHeader& hdr, size_t file_size) noexcept {
    if (hdr.strtab_offset < sizeof(NeoDbHeader) || hdr.index_offset < sizeof(NeoDbHeader) ||
        hdr.template_offset < sizeof(NeoDbHeader)) {
        return false;
    }
    if (hdr.strtab_offset > file_size || hdr.strtab_size > file_size - hdr.strtab_offset) {
        return false;
    }
    const uint64_t idx_bytes = static_cast<uint64_t>(hdr.index_bucket_count) * sizeof(NeoIndexEntry);
    if (hdr.index_bucket_count == 0 || hdr.index_offset > file_size || idx_bytes > file_size - hdr.index_offset) {
        return false;
    }
    if (hdr.template_offset > file_size || hdr.template_size > file_size - hdr.template_offset) {
        return false;
    }
    return (hdr.template_size % sizeof(NeoInstructionTemplate) == 0);
}

} // namespace

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

NeoDatabase::NeoDatabase(NeoDatabase&& other) noexcept
    : mmap_(std::move(other.mmap_)),
      buffer_(other.buffer_),
      header_(other.header_),
      strtab_(other.strtab_),
      index_entries_(other.index_entries_),
      template_data_(other.template_data_) {
    other.buffer_ = {};
    other.header_ = nullptr;
    other.strtab_ = nullptr;
    other.index_entries_ = {};
    other.template_data_ = nullptr;
}

NeoDatabase& NeoDatabase::operator=(NeoDatabase&& other) noexcept {
    if (this != &other) {
        close();
        mmap_ = std::move(other.mmap_);
        buffer_ = other.buffer_;
        header_ = other.header_;
        strtab_ = other.strtab_;
        index_entries_ = other.index_entries_;
        template_data_ = other.template_data_;

        other.buffer_ = {};
        other.header_ = nullptr;
        other.strtab_ = nullptr;
        other.index_entries_ = {};
        other.template_data_ = nullptr;
    }
    return *this;
}

bool NeoDatabase::load_file(const std::filesystem::path& path) {
    close();
    if (!mmap_.open(path) || !validate_and_map(mmap_.bytes())) {
        close();
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
}

const NeoIndexEntry* NeoDatabase::find_entry(std::string_view name) const noexcept {
    if (!header_ || !strtab_ || index_entries_.empty()) {
        return nullptr;
    }
    return find_instruction(name, index_entries_.data(), header_->index_bucket_count,
                            strtab_, header_->strtab_size);
}

std::span<const NeoInstructionTemplate> NeoDatabase::get_templates(
    const NeoIndexEntry& entry) const noexcept {
    if (!header_ || !template_data_ || entry.template_count == 0) {
        return {};
    }
    const uint64_t bytes_needed = static_cast<uint64_t>(entry.template_count) * sizeof(NeoInstructionTemplate);
    if (entry.template_offset + bytes_needed > header_->template_size) {
        return {};
    }
    const auto* ptr = reinterpret_cast<const NeoInstructionTemplate*>(template_data_ + entry.template_offset);
    return {ptr, entry.template_count};
}

const char* NeoDatabase::get_string(uint32_t offset) const noexcept {
    if (!header_ || !strtab_ || offset >= header_->strtab_size) {
        return nullptr;
    }
    const void* term = std::memchr(strtab_ + offset, '\0', header_->strtab_size - offset);
    if (!term) {
        return nullptr;
    }
    return strtab_ + offset;
}

const NeoDbHeader& NeoDatabase::header() const noexcept {
    static constexpr NeoDbHeader s_empty{};
    return header_ ? *header_ : s_empty;
}

bool NeoDatabase::validate_and_map(std::span<const uint8_t> buffer) noexcept {
    if (buffer.size() < sizeof(NeoDbHeader)) {
        return false;
    }
    const auto* hdr = reinterpret_cast<const NeoDbHeader*>(buffer.data());
    if (!validate_header_integrity(*hdr, buffer) || !validate_section_bounds(*hdr, buffer.size())) {
        return false;
    }
    buffer_ = buffer;
    header_ = hdr;
    strtab_ = reinterpret_cast<const char*>(buffer.data() + hdr->strtab_offset);
    index_entries_ = {reinterpret_cast<const NeoIndexEntry*>(buffer.data() + hdr->index_offset),
                      hdr->index_bucket_count};
    template_data_ = buffer.data() + hdr->template_offset;
    return true;
}

} // namespace neo
