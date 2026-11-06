/**
 * @file schema.hpp
 * @brief Packed 8-byte aligned binary domain structures for .neodb opcode databases.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace neo {

/**
 * @brief Categorizes operand bitfield injection roles in instruction templates.
 */
enum NeoOperandType : uint8_t {
    OPERAND_NONE        = 0, ///< Unused operand slot
    OPERAND_REG_GPR     = 1, ///< General purpose register ID
    OPERAND_IMM_INT     = 2, ///< Immediate integer literal
    OPERAND_MEM_DISP    = 3, ///< Base-relative memory displacement
    OPERAND_PIC_REL_TAG = 4, ///< Position-independent relative PC branch displacement
    OPERAND_SPILL_SLOT  = 5  ///< Stack frame spilled variable slot [rbp - offset]
};

#pragma pack(push, 1)

/**
 * @brief Bitfield injection contract for a single instruction operand.
 * Total size: exactly 8 bytes.
 */
struct NeoOperandSlot {
    uint8_t  slot_type;  ///< NeoOperandType discriminator
    uint8_t  bit_offset; ///< Starting bit offset within opcode buffer
    uint8_t  bit_width;  ///< Width of operand field in bits
    uint8_t  flags;      ///< Operand modifiers (sign-extended, scaled, etc.)
    uint32_t mask;       ///< Bitmask applied when injecting operand value
};
static_assert(sizeof(NeoOperandSlot) == 8, "NeoOperandSlot must be exactly 8 bytes");

/**
 * @brief Raw machine instruction opcode template and operand injection slots.
 * Total size: exactly 56 bytes.
 */
struct NeoInstructionTemplate {
    uint16_t       instruction_id;  ///< Canonical instruction identifier
    uint8_t        byte_length;     ///< Machine instruction length in bytes (1-15)
    uint8_t        operand_count;   ///< Number of active operand slots (max 4)
    uint32_t       flags;           ///< PIC/relocation and scratch register flags
    uint8_t        opcode_bytes[16];///< Raw opcode bytes with operand bits zeroed
    NeoOperandSlot slots[4];        ///< Operand injection specifications
};
static_assert(sizeof(NeoInstructionTemplate) == 56, "NeoInstructionTemplate must be 56 bytes");

/**
 * @brief Frozen hash table index entry for O(1) mnemonic lookup.
 * Total size: exactly 24 bytes.
 */
struct NeoIndexEntry {
    uint32_t name_str_offset; ///< Byte offset into string table (.strtab)
    uint32_t name_hash;       ///< 32-bit FNV-1a hash of mnemonic name
    uint32_t flags;           ///< Bit 0: IsOriginQuery, Bit 1: IsMultiInstruction
    uint16_t template_count;  ///< Number of native instruction templates
    uint16_t reserved;        ///< Padding for 8-byte alignment
    uint64_t template_offset; ///< Byte offset into template table (.tmpl)
};
static_assert(sizeof(NeoIndexEntry) == 24, "NeoIndexEntry must be exactly 24 bytes");

/**
 * @brief Master binary header for .neodb opcode databases.
 * Total size: exactly 64 bytes.
 * Section offsets are strictly 8-byte aligned.
 */
struct NeoDbHeader {
    uint8_t  magic[8];           ///< Magic identifier: "NEODB\0\0\0"
    uint32_t format_version;     ///< Format version (0x00010000 for v1.0.0)
    uint16_t arch_id;            ///< Target architecture ID (USX_ARCH_*)
    uint16_t arch_flags;         ///< Bit 0 = Endianness (0=LE, 1=BE), Bit 1 = RegCount (0=16, 1=32)
    uint64_t strtab_offset;      ///< Absolute byte offset to string table (.strtab)
    uint32_t strtab_size;        ///< Total size of string table in bytes
    uint32_t strtab_count;       ///< Total unique strings in string table
    uint64_t index_offset;       ///< Absolute byte offset to index table (.idx)
    uint32_t index_bucket_count; ///< Number of frozen hash buckets (typically 256)
    uint32_t index_entry_count;  ///< Total indexed mnemonic records
    uint64_t template_offset;    ///< Absolute byte offset to template table (.tmpl)
    uint32_t template_size;      ///< Total bytes in template table
    uint32_t header_crc32;       ///< IEEE 802.3 CRC32 checksum over preceding 60 bytes

    /**
     * @brief Computes total instruction templates contained in template section.
     * @return Number of NeoInstructionTemplate records.
     */
    [[nodiscard]] constexpr uint32_t template_count() const noexcept {
        return template_size / sizeof(NeoInstructionTemplate);
    }
};
static_assert(sizeof(NeoDbHeader) == 64, "NeoDbHeader must be exactly 64 bytes");

#pragma pack(pop)

} // namespace neo
