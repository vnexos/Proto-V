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
