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


