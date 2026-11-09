cmake_minimum_required(VERSION 3.20)

# Schema layout verification test
add_executable(test_schema test_schema.cpp)
target_link_libraries(test_schema PRIVATE neoasm_core)
add_test(NAME test_schema COMMAND test_schema)

# RAII Memory mapped file engine unit test
add_executable(test_mmap_unit test_mmap_unit.cpp)
target_link_libraries(test_mmap_unit PRIVATE neoasm_core)
add_test(NAME test_mmap_unit COMMAND test_mmap_unit)

# Common utilities (CRC32, FNV-1a) verification test
add_executable(test_utils test_utils.cpp)
target_link_libraries(test_utils PRIVATE neoasm_core)
add_test(NAME test_utils COMMAND test_utils)

# Zero-copy database reader and index verification test
add_executable(test_mmap test_mmap.cpp)
target_link_libraries(test_mmap PRIVATE neoasm_core)
add_test(NAME test_mmap COMMAND test_mmap)

# YAML to .neodb translator verification test
add_executable(test_translator test_translator.cpp)
target_link_libraries(test_translator PRIVATE neoasm_core)
target_compile_definitions(test_translator PRIVATE NEO_TEMPLATES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../ai-driven/templates")
add_test(NAME test_translator COMMAND test_translator)

# Lossless roundtrip verification test suite (YAML -> neodb -> YAML -> neodb)
add_executable(test_roundtrip test_roundtrip.cpp)
target_link_libraries(test_roundtrip PRIVATE neoasm_core)
target_compile_definitions(test_roundtrip PRIVATE NEO_TEMPLATES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../ai-driven/templates")
add_test(NAME test_roundtrip COMMAND test_roundtrip)

# Assembly Lexer and AST Parser unit test suite
add_executable(test_parser
    test_parser.cpp
    test_parser_instructions.cpp
    test_parser_labels.cpp
    test_parser_errors.cpp
)
target_link_libraries(test_parser PRIVATE neoasm_core)
add_test(NAME test_parser COMMAND test_parser)

# Register virtualization and x86_64 stack spill test suite
add_executable(test_expansion test_expansion.cpp)
target_link_libraries(test_expansion PRIVATE neoasm_core)
add_test(NAME test_expansion COMMAND test_expansion)

# Position-Independent Code (PIC) branch displacement resolver test suite
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_pic_resolver.cpp")
    add_executable(test_pic_resolver test_pic_resolver.cpp)
    target_link_libraries(test_pic_resolver PRIVATE neoasm_core)
    add_test(NAME test_pic_resolver COMMAND test_pic_resolver)
endif()

# Neo-ASM Source Preprocessor and Conditional Directives test suite
add_executable(test_preprocessor
    test_preprocessor.cpp
    test_preprocessor_conditionals.cpp
    test_preprocessor_includes.cpp
)
target_link_libraries(test_preprocessor PRIVATE neoasm_core)
add_test(NAME test_preprocessor COMMAND test_preprocessor)

# Instruction bitfield emitter unit test suite
add_executable(test_emitter test_emitter.cpp)
target_link_libraries(test_emitter PRIVATE neoasm_core)
add_test(NAME test_emitter COMMAND test_emitter)

# Multi-pass assembler integration test suite
add_executable(test_assembler
    test_assembler.cpp
    test_assembler_arithmetic.cpp
    test_assembler_origin.cpp
    test_assembler_pipeline.cpp
    test_assembler_spill_and_pic.cpp
)
target_link_libraries(test_assembler PRIVATE neoasm_core)
target_compile_definitions(test_assembler PRIVATE NEO_TEMPLATES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../ai-driven/templates")
add_test(NAME test_assembler COMMAND test_assembler)

# CLI tool driver integration and smoke test suite
add_executable(test_cli
    test_cli.cpp
    test_cli_diagnostics.cpp
    test_cli_workflows.cpp
)
target_link_libraries(test_cli PRIVATE neoasm_core CLI11::CLI11)
target_compile_definitions(test_cli PRIVATE
    NEO_ASM_EXE="$<TARGET_FILE:neo-asm>"
    NEO_TEMPLATES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../ai-driven/templates"
)
add_test(NAME test_cli COMMAND test_cli)
