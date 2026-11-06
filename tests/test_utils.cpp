/**
 * @file test_utils.cpp
 * @brief Unit tests for IEEE 802.3 CRC32 and 32-bit FNV-1a hashing utilities.
 */

#include <neo/common.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>
#include <string_view>
#include <unordered_set>
#include <vector>

// Compile-time verification of constexpr fnv1a_32 hashing
static_assert(neo::fnv1a_32("") == 0x811c9dc5u, "FNV-1a offset basis mismatch");
static_assert(neo::fnv1a_32("ASN") == 0x59ad78b9u, "FNV-1a hash of ASN mismatch");
static_assert(neo::fnv1a_32("ADD") == 0x7d7558d4u, "FNV-1a hash of ADD mismatch");
static_assert(neo::fnv1a_32("origin(SYSCALL)") == 0x6da35e89u, "FNV-1a origin hash mismatch");

/**
 * @brief Tests IEEE 802.3 CRC32 checksum against standard known test vectors.
 *
 * Verifies standard check value 0xCBF43926 for "123456789" and 0x00000000 for empty input.
 */
static void test_crc32_standard_vectors() {
    const std::string_view input = "123456789";
    const std::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    assert(neo::calcCRC32(data) == 0xCBF43926u);

    const std::span<const uint8_t> empty{};
    assert(neo::calcCRC32(empty) == 0x00000000u);

    const std::string_view fox = "The quick brown fox jumps over the lazy dog";
    assert(neo::calcCRC32(fox) == 0x414FA339u);

    std::cout << "[PASS] test_crc32_standard_vectors\n";
}

/**
 * @brief Tests CRC32 over arbitrary byte buffers and slice ranges.
 */
static void test_crc32_buffer_properties() {
    const uint8_t test_bytes[] = {0x00, 0xFF, 0x55, 0xAA, 0x12, 0x34, 0x56, 0x78};
    const std::span<const uint8_t> full_span(test_bytes, sizeof(test_bytes));
    const uint32_t full_crc = neo::calcCRC32(full_span);
    assert(full_crc != 0x00000000u);

    // Slicing sub-ranges must produce deterministic results matching direct calls
    const std::span<const uint8_t> sub_span = full_span.subspan(1, 4);
    assert(neo::calcCRC32(sub_span) == neo::calcCRC32(std::span<const uint8_t>(test_bytes + 1, 4)));

    std::cout << "[PASS] test_crc32_buffer_properties\n";
}

/**
 * @brief Tests 32-bit FNV-1a mnemonic name hashing for correctness and overload consistency.
 */
static void test_fnv1a_32_mnemonics() {
    assert(neo::fnv1a_32("") == 0x811c9dc5u);
    assert(neo::fnv1a_32("ASN") == 0x59ad78b9u);
    assert(neo::fnv1a_32("ADD") == 0x7d7558d4u);
    assert(neo::fnv1a_32("origin(SYSCALL)") == 0x6da35e89u);

    // Verify string_view and byte span produce identical hashes
    const std::string_view test_str = "origin(SVC 0)";
    const std::span<const uint8_t> byte_span(
        reinterpret_cast<const uint8_t*>(test_str.data()), test_str.size());
    assert(neo::fnv1a_32(test_str) == neo::fnv1a_32(byte_span));

    std::cout << "[PASS] test_fnv1a_32_mnemonics\n";
}

/**
 * @brief Tests signed char sign-extension prevention in FNV-1a calculation.
 */
static void test_fnv1a_32_sign_extension() {
    // Strings containing bytes with MSB set (>= 0x80)
    const char high_bytes_char[] = {'\x80', '\xFF', '\xAA', '\0'};
    const uint8_t high_bytes_u8[] = {0x80, 0xFF, 0xAA};

    const uint32_t hash_from_str = neo::fnv1a_32(std::string_view(high_bytes_char, 3));
    const uint32_t hash_from_u8 = neo::fnv1a_32(std::span<const uint8_t>(high_bytes_u8, 3));
    assert(hash_from_str == hash_from_u8);

    std::cout << "[PASS] test_fnv1a_32_sign_extension\n";
}

/**
 * @brief Tests collision resistance and bucket distribution across Neo-ASM instruction set.
 */
static void test_fnv1a_32_distribution() {
    const std::array<std::string_view, 34> mnemonics = {
        "ASN", "ADD", "SUB", "MUL", "DIV", "MOD", "NEG", "ADD32", "SUB32",
        "AND", "OR", "XOR", "NOT", "SHL", "SHR", "SAR", "CLZ",
        "JMP", "JEQ", "JNE", "JGT", "JGE", "JLT", "JLE",
        "CALL", "RET", "SYS", "BRK", "NOP", "FENCE", "PUSH", "POP",
        "origin(SYSCALL)", "origin(SVC 0)"
    };

    std::unordered_set<uint32_t> hashes;
    std::array<uint32_t, 256> buckets{};

    for (std::string_view m : mnemonics) {
        const uint32_t h = neo::fnv1a_32(m);
        assert(hashes.insert(h).second); // Must be unique: zero collisions in instruction set
        buckets[h % 256]++;
    }

    // Verify low clustering across 256 buckets (max entries per bucket <= 3)
    for (uint32_t count : buckets) {
        assert(count <= 3);
    }

    std::cout << "[PASS] test_fnv1a_32_distribution\n";
}

int main() {
    test_crc32_standard_vectors();
    test_crc32_buffer_properties();
    test_fnv1a_32_mnemonics();
    test_fnv1a_32_sign_extension();
    test_fnv1a_32_distribution();

    std::cout << "All CRC32 and FNV-1a utility tests passed successfully.\n";
    return 0;
}
