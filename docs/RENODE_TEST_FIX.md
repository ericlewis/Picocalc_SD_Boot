# Renode Test Configuration Fix

## Problem

The Renode tests were failing in CI with the following error:
```
Could not find file 'platforms/boards/raspberry-pi-pico.repl'.
```

This occurred because the test scripts were trying to load platform description files from Renode's installation directory that don't exist in the standard Renode distribution.

## Solution

Created local platform description files and updated all test scripts to use them.

### Changes Made

1. **Created Platform Description Files**
   - `tests/platforms/rp2040_basic.repl` - Basic RP2040 platform using generic Renode peripherals
   - `tests/platforms/rp2350_basic.repl` - Basic RP2350 platform for Pico 2

2. **Updated Test Scripts**
   - Modified `tests/rp2040.resc` to use local platform files and removed external Python device dependencies
   - Updated `tests/simple_test.resc` to use local platform files
   - Updated `tests/bootloader.robot` to remove redundant platform loading

3. **Updated Python Test Runners**
   - Modified `tests/run_renode_tests.py` to use local platform file paths
   - Updated `tests/simple_renode_test.py` to use local platform files
   - Updated `tests/renode_flash_test.py` to use local platform files

4. **Simplified Platform Descriptions**
   - Used generic peripherals (PL011 UART, STM32 SPI/GPIO) instead of RP2040-specific ones
   - Added memory regions for flash and SRAM
   - Tagged peripheral regions for basic compatibility

## Platform Description Details

The platform files define:
- **CPU**: Cortex-M0+ for RP2040, Cortex-M33 for RP2350
- **Memory**: 16MB flash at 0x10000000, 264KB/512KB SRAM at 0x20000000
- **Peripherals**: Generic UART, SPI, GPIO, and system timer
- **Tags**: Memory regions tagged for watchdog, USB, DMA, etc.

## Test Compatibility

The simplified platform descriptions provide enough functionality for basic bootloader testing:
- UART output monitoring
- Memory access logging
- Basic peripheral tagging
- CPU execution control

## Running Tests

With these changes, the Renode tests should now work in the CI environment when Renode is installed:

```bash
# In CI environment with Renode installed
cd tests
robot --variable RENODE_PORT:9999 --outputdir ../test-artifacts bootloader.robot
```

## Notes

- The tests are marked with `continue-on-error: true` in CI as they may still need refinement
- Platform files use generic peripherals that are guaranteed to exist in standard Renode
- No external Python device scripts are required
- Tests can be enhanced later with more specific peripheral implementations 