cmake_minimum_required(VERSION 3.20)

# Schema layout verification test
add_executable(test_schema test_schema.cpp)
target_include_directories(test_schema PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_features(test_schema PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(test_schema PRIVATE /W4 /permissive-)
else()
    target_compile_options(test_schema PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME test_schema COMMAND test_schema)

# RAII Memory mapped file engine unit test
add_executable(test_mmap_unit test_mmap_unit.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/mmap.cpp)
target_include_directories(test_mmap_unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_features(test_mmap_unit PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(test_mmap_unit PRIVATE /W4 /permissive-)
else()
    target_compile_options(test_mmap_unit PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME test_mmap_unit COMMAND test_mmap_unit)

# Common utilities (CRC32, FNV-1a) verification test
add_executable(test_utils
    test_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/crc32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/hash.cpp
)
target_include_directories(test_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_features(test_utils PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(test_utils PRIVATE /W4 /permissive-)
else()
    target_compile_options(test_utils PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME test_utils COMMAND test_utils)

# Zero-copy database reader and index verification test
add_executable(test_mmap
    test_mmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/database/database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/mmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/crc32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/hash.cpp
)
target_include_directories(test_mmap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_features(test_mmap PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(test_mmap PRIVATE /W4 /permissive-)
else()
    target_compile_options(test_mmap PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME test_mmap COMMAND test_mmap)

