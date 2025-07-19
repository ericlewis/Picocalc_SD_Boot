# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the Picocalc SD Bootloader - a high-memory bootloader for Raspberry Pi Pico (RP2040) and Pico 2 W (RP2350) that enables loading and flashing firmware directly from SD card without USB connection. Originally developed for the ClockworkPi PicoCalc platform, this bootloader supports dynamic firmware loading and provides a foundation for multi-boot scenarios.

### Key Context from ClockworkPi Community
- Part of the PicoCalc ecosystem for flexible firmware updates
- Enables chainloading different applications without USB connection
- Community interest in features like auto-boot menus (GRUB-style) and multi-stage boot processes
- Designed to support experimentation with different firmware builds
- We are specifically targeting the PicoCalc, so the forum is probably helpful too: https://forum.clockworkpi.com/

## Build Commands

### Building the Bootloader
```bash
# Build for Pico (RP2040)
cmake -B build -DPICO_BOARD=pico && cmake --build build

# Build for Pico W
cmake -B build -DPICO_BOARD=pico_w && cmake --build build

# Build for Pico 2 W (RP2350)
cmake -B build -DPICO_BOARD=pico2_w && cmake --build build

# Clean build
rm -rf build
```

Output files are in `build/` directory: `.uf2`, `.elf`, `.bin`, `.hex`

### Running Tests
```bash
# Run all tests (unit tests + test file generation)
./tools/run_tests_cmake.sh

# Build and run tests manually
mkdir -p build_tests
cd build_tests
cmake ../tests -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
ctest --output-on-failure

# Run specific test
./build_tests/test_uf2
./build_tests/test_critical_scenarios
./build_tests/test_safety_gaps
```

### Linting and Type Checking
```bash
# No automatic lint/typecheck commands configured
# Ask user for appropriate commands if needed
# Suggest writing them to CLAUDE.md for future reference
```

### Local CI Testing
```bash
# Run GitHub Actions locally with act
./run_workflows_locally.sh build  # Build for all boards
./run_workflows_locally.sh unit   # Run unit tests
./run_workflows_locally.sh pico   # Build for pico only
```

## High-Level Architecture

### Memory Layout
The bootloader uses a high-memory layout to preserve standard application space:
- **RP2040**: 140KB bootloader at end of 2MB flash (leaves 0x10000000 free for applications)
- **RP2350**: Uses ATU (Address Translation Unit) for memory mapping
- Custom linker scripts: `bootloader/memmap_2040.ld` and `bootloader/memmap_2350.ld`

### Core Components

1. **Flash Operations** (`bootloader/src/flash_ops.c`)
   - All flash operations run from RAM for safety
   - Implements power failure recovery mechanism
   - CRC32 verification after every flash operation
   - Atomic prog_info updates only after successful flash

2. **UF2 Handler** (`bootloader/src/uf2_handler.c`)
   - Validates UF2 blocks before processing
   - Protects bootloader memory regions
   - Preserves boot2 sector
   - Handles both RP2040 and RP2350 family IDs

3. **UI System** (`bootloader/src/ui.c` + `bootloader/libs/`)
   - I2C keyboard driver for input
   - SPI LCD driver with built-in fonts
   - Text-based file browser
   - Error display and user feedback

4. **SD Card Interface** (`bootloader/src/sd_card.c`)
   - Supports SDHC/SDXC cards
   - FAT32 and exFAT filesystem support via pico-vfs
   - SPI mode communication

### Critical Safety Mechanisms

The bootloader implements multiple layers of protection:
1. **Vector Table Validation**: Checks valid stack pointer and reset vector before boot
2. **Bootloader Boundary Protection**: Prevents self-overwriting
3. **Boot2 Preservation**: Never overwrites the boot2 sector
4. **Flash Write Verification**: CRC32 check after every flash operation
5. **Power Failure Recovery**: Detects and recovers from incomplete flash operations

### Pin Assignments

**SD Card (SPI0)**:
- CLK: GPIO 18
- MOSI: GPIO 19
- MISO: GPIO 16
- CS: GPIO 22

**LCD (SPI1)**:
- CLK: GPIO 10
- MOSI: GPIO 11
- CS: GPIO 13
- DC: GPIO 15
- RESET: GPIO 14

**Keyboard (I2C1)**:
- SDA: GPIO 6
- SCL: GPIO 7

### Build System

The project uses CMake as the primary build system with PlatformIO for additional testing environments. Key configuration files:
- `CMakeLists.txt`: Main build configuration
- `platformio.ini`: Alternative build and test environments
- Board definitions in `pico-sdk/src/boards/include/boards/`

### Testing Infrastructure

Tests are organized into:
- **Unit Tests**: Component-level testing of core functions
- **Critical Scenario Tests**: Edge cases and failure modes
- **Safety Gap Tests**: Verification of all safety mechanisms
- **Test File Generators**: Create valid and corrupted UF2 files for testing

## Important Development Notes

1. **Flash Operations Must Run from RAM**: Any function that erases or writes flash must be marked with `__no_inline_not_in_flash_func` or run from a RAM-based copy.

2. **Platform Differences**: 
   - RP2040 uses custom boot2, RP2350 uses SDK boot2
   - Different flash sizes and memory layouts
   - Family ID handling in UF2 blocks

3. **BOOTSEL Functionality**: Hardware-based recovery always available - holding BOOTSEL during reset/power-on enters USB boot mode regardless of bootloader state.

4. **No Renode Support**: Renode emulation was removed due to incompatibility. All testing should be done on hardware or with unit tests.

5. **CYW43 Wireless**: On Pico W variants, wireless chip uses PIO-based SPI (no conflict with bootloader peripherals).

## Bootloader Architecture Details

### Boot Sequence
1. **Boot2 Stage**: Custom boot2 (`bootloader/boot2/boot2_custom.S`) configures QSPI flash for XIP mode
2. **High-Memory Bootloader**: Main bootloader executes from end of flash (last 140KB on RP2040)
3. **Application Detection**: Checks for magic number at end of flash to detect bootloader presence
4. **UI Initialization**: Sets up I2C keyboard and SPI LCD for user interaction
5. **File Selection**: User browses SD card and selects UF2 file
6. **Flash Update**: Validates and flashes selected firmware while preserving bootloader

### Memory Protection Strategy
- Bootloader stores magic number (`0xe98cc638`) and start address in last 8 bytes of flash
- Applications can query this to determine available flash space
- Prevents accidental bootloader corruption by applications
- Boot2 sector is always preserved during updates

## Future Enhancement Ideas (from Community)

### Auto-Boot Menu System
- Implement timeout-based auto-boot (like GRUB)
- Store boot preferences in flash or SD card
- Support multiple firmware profiles

### Multi-Stage Boot Process
- Support for stage3 bootloader loading from SD card
- Chain-loading different bootloaders
- Dynamic bootloader updates without USB

### Enhanced Features
- Data restore functionality for flash memory
- Support for compressed firmware images
- Network boot capability (for Pico W variants)
- Dual-boot support with fallback options

## Credits and Resources

- **Original SD Boot Concept**: Hiroyuki Oyama (VFS implementation)
- **High-Memory Implementation**: muzkr
- **RP2350 Port**: TheKiwil
- **Blog**: [Writing Custom Bootloader for RPI Pico](https://hsuanhanlai.com/writting-custom-bootloader-for-RPI-Pico/)
- **Forum**: [ClockworkPi Discussion](https://forum.clockworkpi.com/t/uf2-loader-release/18479)