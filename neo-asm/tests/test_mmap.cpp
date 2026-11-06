/**
 * @file test_mmap.cpp
 * @brief Tests verifying zero-copy .neodb loading, header validation, and O(1) index queries.
 */

#include <neo/common.hpp>
#include <neo/database.hpp>
#include <neo/schema.hpp>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

static_assert(sizeof(neo::NeoDbHeader) == 64, "NeoDbHeader must be 64 bytes");

namespace {

std::vector<uint8_t> create_mock_neodb() {
    constexpr uint32_t bucket_count = 256;
    constexpr uint64_t strtab_off = 64, idx_off = 80;
    constexpr uint64_t idx_size = bucket_count * sizeof(neo::NeoIndexEntry);
    constexpr uint64_t tmpl_off = idx_off + idx_size;
    constexpr uint32_t tmpl_size = 2 * sizeof(neo::NeoInstructionTemplate);

    std::vector<uint8_t> buf(tmpl_off + tmpl_size, 0);
    auto* hdr = reinterpret_cast<neo::NeoDbHeader*>(buf.data());
    std::memcpy(hdr->magic, "NEODB\0\0\0", 8);
    hdr->format_version = 0x00010000;
    hdr->arch_id = neo::USX_ARCH_X86_64;
    hdr->strtab_offset = strtab_off;
    hdr->strtab_size = 16;
    hdr->strtab_count = 2;
    hdr->index_offset = idx_off;
    hdr->index_bucket_count = bucket_count;
    hdr->index_entry_count = 2;
    hdr->template_offset = tmpl_off;
    hdr->template_size = tmpl_size;

    std::memcpy(buf.data() + strtab_off, "\0ADD\0SUB\0", 9);

    auto* entries = reinterpret_cast<neo::NeoIndexEntry*>(buf.data() + idx_off);
    const uint32_t h_add = neo::fnv1a_32("ADD");
    entries[h_add % bucket_count] = {1, h_add, 0, 1, 0, 0};
    const uint32_t h_sub = neo::fnv1a_32("SUB");
    entries[h_sub % bucket_count] = {5, h_sub, 0, 1, 0, sizeof(neo::NeoInstructionTemplate)};

    auto* tmpls = reinterpret_cast<neo::NeoInstructionTemplate*>(buf.data() + tmpl_off);
    tmpls[0] = {1, 3, 2, 0, {0x48, 0x01, 0xD8}, {}};
    tmpls[1] = {2, 3, 2, 0, {0x48, 0x29, 0xD8}, {}};

    hdr->header_crc32 = neo::calcCRC32(std::span<const uint8_t>(buf.data(), 60));
    return buf;
}

void test_valid_load_and_pointer_containment() {
    const auto temp_path = std::filesystem::temp_directory_path() / "test_valid.neodb";
    const auto buf = create_mock_neodb();
    {
        std::ofstream out(temp_path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    }
    neo::NeoDatabase db;
    assert(db.load_file(temp_path));
    assert(db.is_loaded() && db.header().format_version == 0x00010000);

    const auto* entry = db.find_entry("ADD");
    assert(entry != nullptr);
    const auto* entry_b = reinterpret_cast<const uint8_t*>(entry);
    assert(entry_b >= db.raw_bytes().data() && entry_b < db.raw_bytes().data() + db.raw_bytes().size());

    const char* str = db.get_string(entry->name_str_offset);
    assert(str && std::string_view(str) == "ADD");
    const auto* str_b = reinterpret_cast<const uint8_t*>(str);
    assert(str_b >= db.raw_bytes().data() && str_b < db.raw_bytes().data() + db.raw_bytes().size());

    const auto tmpls = db.get_templates(*entry);
    assert(tmpls.size() == 1 && tmpls[0].instruction_id == 1);
    const auto* tmpl_b = reinterpret_cast<const uint8_t*>(tmpls.data());
    assert(tmpl_b >= db.raw_bytes().data() && tmpl_b < db.raw_bytes().data() + db.raw_bytes().size());

    assert(db.find_entry("NON_EXISTENT") == nullptr);
    std::filesystem::remove(temp_path);
}

void test_header_integrity_validation() {
    auto buf = create_mock_neodb();
    neo::NeoDatabase db;

    auto* hdr = reinterpret_cast<neo::NeoDbHeader*>(buf.data());
    std::memcpy(hdr->magic, "BADDB\0\0\0", 8);
    assert(!db.load_buffer(buf));

    buf = create_mock_neodb();
    hdr = reinterpret_cast<neo::NeoDbHeader*>(buf.data());
    hdr->format_version = 0x00020000;
    hdr->header_crc32 = neo::calcCRC32(std::span<const uint8_t>(buf.data(), 60));
    assert(!db.load_buffer(buf));

    buf = create_mock_neodb();
    hdr = reinterpret_cast<neo::NeoDbHeader*>(buf.data());
    hdr->header_crc32 ^= 0xFFFFFFFF;
    assert(!db.load_buffer(buf));

    assert(!db.load_buffer(std::span<const uint8_t>(buf.data(), 40)));
}

void test_alignment_and_bounds_validation() {
    neo::NeoDatabase db;
    for (int i = 0; i < 3; ++i) {
        auto buf = create_mock_neodb();
        auto* hdr = reinterpret_cast<neo::NeoDbHeader*>(buf.data());
        if (i == 0) hdr->strtab_offset += 1;
        if (i == 1) hdr->index_offset += 2;
        if (i == 2) hdr->template_offset += 3;
        hdr->header_crc32 = neo::calcCRC32(std::span<const uint8_t>(buf.data(), 60));
        assert(!db.load_buffer(buf));
    }
    auto buf = create_mock_neodb();
    auto* hdr = reinterpret_cast<neo::NeoDbHeader*>(buf.data());
    hdr->template_size = 1000000;
    hdr->header_crc32 = neo::calcCRC32(std::span<const uint8_t>(buf.data(), 60));
    assert(!db.load_buffer(buf));
}

void test_linear_probing_and_move() {
    auto buf = create_mock_neodb();
    auto* hdr = reinterpret_cast<neo::NeoDbHeader*>(buf.data());
    auto* entries = reinterpret_cast<neo::NeoIndexEntry*>(buf.data() + hdr->index_offset);

    // Adjacent bucket simulation for linear probe collision
    const uint32_t h_add = neo::fnv1a_32("ADD");
    const uint32_t probe_idx = (h_add % hdr->index_bucket_count + 1) % hdr->index_bucket_count;
    entries[probe_idx] = {5, h_add, 0, 1, 0, sizeof(neo::NeoInstructionTemplate)};

    neo::NeoDatabase db;
    assert(db.load_buffer(buf));
    const auto* entry = db.find_entry("ADD");
    assert(entry && entry->template_offset == 0);

    neo::NeoDatabase moved_db(std::move(db));
    assert(!db.is_loaded() && moved_db.is_loaded());
    assert(moved_db.find_entry("ADD") != nullptr);

    moved_db.close();
    assert(!moved_db.is_loaded() && moved_db.find_entry("ADD") == nullptr);
}

} // namespace

int main() {
    test_valid_load_and_pointer_containment();
    test_header_integrity_validation();
    test_alignment_and_bounds_validation();
    test_linear_probing_and_move();
    std::cout << "[PASS] test_mmap completed successfully.\n";
    return 0;
}
