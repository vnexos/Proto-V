cmake_minimum_required(VERSION 3.20)
project(ProtoV VERSION 1.0.0 LANGUAGES CXX)

# Enforce C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch external dependencies via FetchContent
include(FetchContent)

# Fetch yaml-cpp v0.9.0 for YAML architecture specification parsing
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        yaml-cpp-0.9.0
    GIT_SHALLOW    TRUE
)
set(YAML_CPP_BUILD_TESTS OFF CACHE INTERNAL "")
set(YAML_CPP_BUILD_TOOLS OFF CACHE INTERNAL "")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(yaml-cpp)

# Fetch CLI11 v2.7.2 for command-line flag and subcommand parsing
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.7.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(cli11)

# Set compiler warning flags for Proto-V project targets
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core static library for Neo-ASM subsystem
add_library(neoasm_core STATIC
    src/common/crc32.cpp
    src/common/hash.cpp
    src/common/mmap.cpp
    src/database/database.cpp
    src/database/template_builder.cpp
    src/database/yaml_reader.cpp
    src/database/database_writer.cpp
    src/database/yaml_ops_emitter.cpp
    src/database/yaml_decompiler.cpp
    src/database/translator.cpp
)
target_include_directories(neoasm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(neoasm_core PUBLIC yaml-cpp)
target_compile_features(neoasm_core PUBLIC cxx_std_20)

# Optional source directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt")
    add_subdirectory(src)
endif()

# Enable test registration and execution via CTest
enable_testing()
add_subdirectory(tests)
