cmake_minimum_required(VERSION 3.13)
project(picocalc_sd_boot_tests C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../bootloader/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../bootloader/libs
    ${CMAKE_CURRENT_SOURCE_DIR}/../pico-vfs/include
)

# Common test sources
set(COMMON_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../bootloader/src/error_handler.c
)

# Test 1: Error codes test
add_executable(test_error_codes
    test_error_codes.c
    ${COMMON_TEST_SOURCES}
)
target_compile_definitions(test_error_codes PRIVATE
    PICO_BUILD=1
    TESTING=1
)

# Test 2: UF2 implementation test
add_executable(test_uf2
    test_uf2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../bootloader/src/uf2.c
)
target_compile_definitions(test_uf2 PRIVATE
    PICO_BUILD=1
    TESTING=1
)

# Test 3: Critical scenarios test
add_executable(test_critical_scenarios
    test_critical_scenarios.c
)
target_compile_definitions(test_critical_scenarios PRIVATE
    PICO_BUILD=1
    TESTING=1
)

# Test 4: Safety gaps test
add_executable(test_safety_gaps
    test_safety_gaps.c
)
target_compile_definitions(test_safety_gaps PRIVATE
    PICO_BUILD=1
    TESTING=1
)

# Add tests to CTest
add_test(NAME error_codes COMMAND test_error_codes)
add_test(NAME uf2 COMMAND test_uf2)
add_test(NAME critical_scenarios COMMAND test_critical_scenarios)
add_test(NAME safety_gaps COMMAND test_safety_gaps)

# Create test runner script
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
"#!/bin/bash
echo 'Running all tests...'
ctest --output-on-failure
")
file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)