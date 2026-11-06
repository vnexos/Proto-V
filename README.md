# Proto-V: Neo-ASM Toolchain

Proto-V is a high-performance C++20 systems architecture featuring **Neo-ASM**, a zero-copy memory-mapped opcode database and intermediate assembly compiler pipeline.

## Features
- **Zero-Copy Architecture**: Memory-mapped binary database (`.neodb`) with O(1) frozen hash lookup.
- **Cross-Architecture Support**: Native code emission for `x86_64`, `aarch64`, and `riscv64`.
- **Bidirectional Toolchain**:
  - `neoasm --translate <arch>.yaml <arch>.neodb`: Convert human-readable YAML specifications to binary opcode databases and vice versa.
  - `neoasm --compile <input.neoa> <output.bin> --arch=<arch>`: Compile Neo-ASM intermediate source to pure flat machine code binaries.
