# Picocalc SD Bootloader

`Picocalc_SD_Boot` is a custom, high-memory bootloader for the Raspberry Pi Pico (RP2040) and Pico 2 W (RP2350). It allows you to load and flash `.uf2` firmware images directly from an SD card, providing a fast and convenient way to update your device without needing to connect it to a computer.

<div align="center">
    <img src="img/sd_boot.jpg" alt="Picocalc SD Bootloader in action" width="80%">
</div>

## Features

- **Dual-Platform Support:** Fully compatible with both the RP2040 (Pico) and the new RP2350 (Pico 2 W).
- **High-Memory Layout:** The bootloader resides in the upper portion of the flash memory, leaving the standard `0x10000000` address space free for your application.
- **RP2350 ATU Support:** Utilizes the RP2350's Address Translation Unit (ATU) to seamlessly map the application into the standard memory space.
- **Auto-Boot Menu System:** GRUB-style boot menu with configurable timeout for automatic boot selection.
- **Boot Configuration:** Customizable boot behavior via `/boot.conf` file on SD card.
- **Polished User Interface:** A clean, intuitive text-based UI for easy navigation and file selection, complete with a decorative frame and clear on-screen instructions.
- **Robust Flashing:** Features a hardened flash writer with pre-flash validation and post-flash CRC32 verification to ensure data integrity.
- **SD Card Flexibility:** Supports both SDHC and SDXC cards with FAT32 or exFAT filesystems, including a high-speed mode for faster flashing.
- **Comprehensive Error Handling:** Detailed error messages with recovery instructions for common issues.

## Building the Bootloader

To build the bootloader, you will need the Pico SDK and a GCC ARM toolchain.

1.  **Clone the repository and initialize submodules:**
    ```bash
    git clone https://github.com/adwuard/Picocalc_SD_Boot.git
    cd Picocalc_SD_Boot
    git submodule update --init --recursive
    ```

2.  **Create a build directory:**
    ```bash
    mkdir build && cd build
    ```

3.  **Configure the build for your target platform:**

    *   **For PicoCalc (RP2040-based):**
        ```bash
        cmake -DPICO_BOARD=picocalc ..
        ```

    *   **For standard Pico (RP2040):**
        ```bash
        cmake -DPICO_BOARD=pico ..
        ```

    *   **For Pico W (RP2040 with wireless):**
        ```bash
        cmake -DPICO_BOARD=pico_w ..
        ```

    *   **For Pico 2 W (RP2350):**
        ```bash
        cmake -DPICO_BOARD=pico2_w ..
        ```

4.  **Build the bootloader:**
    ```bash
    cmake --build . -j$(nproc)
    ```
    The compiled `.uf2` file will be located in the `build/` directory.

### PicoCalc-Specific Notes

The PicoCalc board definition includes all the specific pin configurations for:
- I2C keyboard on I2C1 (SDA: GPIO6, SCL: GPIO7, Address: 0x1F)
- SPI LCD display on SPI1 (320x320 ILI9488)
- SD card on SPI0 with card detect on GPIO22
- Audio output pins on GPIO27/28
- Control buttons on GPIO2/3

Building with `-DPICO_BOARD=picocalc` automatically configures all these peripherals correctly.

## Auto-Boot Menu System

The bootloader now features a GRUB-style auto-boot menu that appears on startup:

### Boot Options
1. **Boot Last Application** - Launches the last flashed firmware from internal flash
2. **Select Firmware from SD** - Opens the SD card file browser to select a `.uf2` file
3. **Enter USB Boot Mode** - Enters USB mass storage mode for firmware updates

### Configuration

Create a `boot.conf` file in the root of your SD card to customize boot behavior:

```ini
# Boot timeout in seconds (0 = no timeout, max 30)
timeout=5

# Default boot option:
# 0 = Boot last application from flash
# 1 = Select firmware from SD card
# 2 = Enter USB boot mode
default=0

# Show countdown timer (1 = yes, 0 = no)
show_countdown=1

# Default firmware file to load from SD (if default=1)
# Path is relative to /firmware directory
default_firmware=my_app.uf2
```

### User Interaction
- **Arrow Keys** - Navigate menu options
- **Enter/Right Arrow** - Select option
- **ESC/Left Arrow** - Cancel and boot default
- **Any Key** - Interrupt auto-boot countdown

The menu will automatically boot the default option after the timeout expires unless interrupted by a key press.

## Bootloader Self-Update

The bootloader can update itself from an SD card without requiring USB connection:

### Creating Update Files

Use the provided tool to create bootloader update packages (.bup files):

```bash
# Create update package from UF2 file
python3 tools/create_bootloader_update.py picocalc_sd_boot.uf2 -v 1.3.0 -p rp2040
```

This creates a file like `picocalc_sd_boot_1.3.0_rp2040.bup`

### Installing Updates

1. Copy the `.bup` file to the `/firmware` directory on your SD card
2. Boot the PicoCalc and enter the SD card menu
3. Select the `.bup` file (it will show as "BL UPD" in the size column)
4. Confirm the update when prompted
5. The device will automatically reboot with the new bootloader

### Safety Features

- CRC32 verification before and after flashing
- Platform compatibility checks (RP2040 vs RP2350)
- Version information in update files
- Warning messages to prevent power loss during update
- Update process runs entirely from RAM for safety

**WARNING**: Do not power off the device during bootloader update!

## Firmware Integrity Verification

The bootloader performs comprehensive integrity checks on UF2 firmware files:

### Pre-Flash Validation
Before any flash operations begin:
- Validates all UF2 block magic numbers and structure
- Checks family ID compatibility (RP2040 vs RP2350)
- Verifies target addresses are within valid ranges
- Ensures no bootloader overwrite attempts
- Calculates CRC32 of all payload data

### Post-Flash Verification
- After each block write, verifies data was written correctly
- Final CRC32 check compares flashed data with pre-calculated checksum
- Any mismatch triggers an error and prevents boot

### Benefits
- **Early Detection**: Corrupted files detected before flash operations
- **No Partial Updates**: Failed verification prevents incomplete firmware
- **Data Integrity**: Ensures firmware matches original file exactly
- **Clear Feedback**: "Validating firmware..." and "Verifying flash..." status messages

## Technical Implementation

### Bootloader Detection and Size Management

The bootloader uses the last 8 bytes of flash to store a magic number (`0xe98cc638`) and the bootloader's start address. This allows applications to detect the presence of the bootloader at runtime and determine the available application space, preventing accidental corruption.

### Flash Update Mechanism

The flash update process is designed for safety and reliability:
- The bootloader itself is never overwritten during an update.
- The application area is erased and programmed using the pico-sdk's `flash_range_erase` and `flash_range_program` functions.
- All data written to flash is verified with a CRC32 checksum to ensure integrity.

## Credits

- **Hiroyuki Oyama:** For the original SD card bootloader mechanism and VFS implementation.
- **TheKiwil:** For contributions to the RP2350 port, including the custom linker script.
- **muzkr:** For the initial high-memory bootloader implementation.

## Further Reading

- **Blog Post:** For a detailed technical write-up, see the [project blog page](https://hsuanhanlai.com/writting-custom-bootloader-for-RPI-Pico/).
- **Forum Discussion:** Join the conversation on the [Clockwork Pi Forum](https://forum.clockworkpi.com/t/i-made-an-app-that-dynamically-load-firmware-from-sd-card/16664/24).

## Local CI/CD Testing with `act`

You can run the full CI/CD pipeline locally using [act](https://github.com/nektos/act). This allows you to test your changes in an environment that mirrors the GitHub Actions runners.

1.  **Install `act`:** Follow the official [installation instructions](https://github.com/nektos/act#installation).

2.  **Build the custom Docker image:**
    ```bash
    docker build -t picocalc-boot-env:latest -f docker/Dockerfile .
    ```

3.  **Run the `emulation-test` job:**
    ```bash
    act -j emulation-test --container-architecture linux/amd64 -P ubuntu-latest=picocalc-boot-env:latest
    ```
    This command will execute the `emulation-test` job defined in `.github/workflows/build.yml` using your local Docker image.
