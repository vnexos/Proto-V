# Neo-ASM Syntax Highlighting for VS Code

Official Visual Studio Code extension providing syntax highlighting, language configuration, and code snippets for **Neo-ASM** (`.neoa`) intermediate assembly files in the **Proto-V** project.

## Features

- **Full Syntax Highlighting**:
  - Unified data movement: `ASN`, `PUSH`, `POP`.
  - 64-bit and 32-bit arithmetic: `ADD`, `SUB`, `MUL`, `DIV`, `MOD`, `NEG`, `UDIV`, `UMOD`, `ADD32`, etc.
  - Bitwise operations: `AND`, `OR`, `XOR`, `NOT`, `SHL`, `SHR`, `SAR`, `CLZ`, etc.
  - Position-Independent branches: `JMP`, `JEQ`, `JNE`, `JGT`, `JGE`, `JLT`, `JLE`, `JUGT`, `JUGE`, `JULT`, `JULE`, `CALL`, `RET`.
  - Native architecture queries: `origin(...)` passthrough.
  - Virtual 32-register model: `r0`–`r31`, system aliases `sp`, `fp`, `zr`.
  - Memory operands: `[reg + disp]` bracketed syntax.
- **Comments & Formatting**: Automatic comment toggling for `;`, `//`, `#`, and `/* ... */`.
- **Brackets & Autoclosing**: Intelligent matching for `[]`, `()`, `{}` and quotes.
- **Code Snippets**: Quick templates for Neo-ASM entrypoints, conditional branches, and memory instructions.

## Installation

This extension is located under `tools/vscode-neo-asm/` in Proto-V. It is also linked into `~/.vscode/extensions/protov.neo-asm-syntax-0.1.0/` for immediate use in VS Code.
