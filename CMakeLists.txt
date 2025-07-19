cmake_minimum_required(VERSION 3.13)

# Set PICO_SDK_PATH to the location of the Pico SDK
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk")

# Include the Pico SDK (must be before project)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set the project name and languages
project(picocalc_sd_boot C CXX ASM)

# Initialize the Pico SDK
pico_sdk_init()

# Set the C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optimization flags for size
add_compile_options(-Os -ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections)

# Custom libraries
add_subdirectory(bootloader/libs/i2ckbd)
add_subdirectory(bootloader/libs/lcdspi)

# pico-vfs library - include as interface library
include(pico-vfs/pico_vfs.cmake)

# Create pico-vfs library
add_library(pico-vfs INTERFACE)
target_sources(pico-vfs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/src/blockdevice/sd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/src/blockdevice/heap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/src/blockdevice/loopback.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/src/filesystem/fat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/src/filesystem/littlefs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/src/filesystem/vfs.c
    # Add FatFS source files
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/vendor/ff15/source/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/vendor/ff15/source/ffsystem.c
    # Note: ffunicode.c removed to save space
    # Add littlefs source files
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/vendor/littlefs/lfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/vendor/littlefs/lfs_util.c
)
target_include_directories(pico-vfs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/include/filesystem/ChaN
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/vendor/littlefs
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-vfs/vendor/ff15/source
)
target_link_libraries(pico-vfs INTERFACE
    hardware_spi
    pico_sync
)

# Main executable
add_executable(picocalc_sd_boot
    bootloader/src/main.c
    bootloader/src/uf2.c
    bootloader/src/proginfo.c
    bootloader/src/bldetect.c
    bootloader/src/key_event.c
    bootloader/src/text_directory_ui.c
    bootloader/src/ff_minimal_unicode.c
    bootloader/src/error_handler.c
    bootloader/src/auto_boot.c
    bootloader/src/bootloader_update.c
)

# Only use custom boot2 for RP2040, use SDK default for RP2350
if(PICO_PLATFORM STREQUAL "rp2040")
    target_sources(picocalc_sd_boot PRIVATE bootloader/boot2/boot2_custom.S)
    # Disable SDK boot2
    set(PICO_DEFAULT_BOOT_STAGE2_FILE "" CACHE STRING "")
    set(PICO_DEFAULT_BOOT_STAGE2 "" CACHE STRING "")
endif()

# Set linker script based on target platform
# Only set custom linker script for RP2040
if(PICO_PLATFORM STREQUAL "rp2040")
    pico_set_linker_script(picocalc_sd_boot ${CMAKE_CURRENT_SOURCE_DIR}/bootloader/memmap_2040.ld)
endif()
# For RP2350, use SDK default linker script
# TODO: Fix custom linker script to handle binary info placement for RP2350

# Link SDK and custom libs
target_link_libraries(picocalc_sd_boot
    pico_stdlib
    hardware_flash
    pico_unique_id
    boot_uf2_headers
    i2ckbd
    lcdspi
    pico-vfs
)

# Include paths
target_include_directories(picocalc_sd_boot PRIVATE
    bootloader/include
    pico-vfs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bootloader/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bootloader/src
    ${CMAKE_CURRENT_SOURCE_DIR}/bootloader/libs
)

# Enable testing and add tests (build separately as host executables)
# Tests should be built with: 
# mkdir build_tests && cd build_tests
# cmake ../tests -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
# make && ctest --output-on-failure

pico_add_extra_outputs(picocalc_sd_boot) 