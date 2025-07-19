/**
 * PicoCalc SD Firmware Loader - Bootloader Self-Update
 * 
 * Allows the bootloader to update itself from an SD card file
 * without requiring USB connection.
 */

#ifndef BOOTLOADER_UPDATE_H
#define BOOTLOADER_UPDATE_H

#include <stdint.h>
#include <stdbool.h>
#include "error_codes.h"

// Bootloader update file magic number
#define BOOTLOADER_UPDATE_MAGIC 0x42554C50  // "BULP" - Bootloader Update Package

// Maximum bootloader size (must match linker script)
#if PICO_RP2040
#define MAX_BOOTLOADER_SIZE (140 * 1024)  // 140KB for RP2040
#else
#define MAX_BOOTLOADER_SIZE (256 * 1024)  // 256KB for RP2350
#endif

// Bootloader update header structure
typedef struct {
    uint32_t magic;             // BOOTLOADER_UPDATE_MAGIC
    uint32_t version;           // Bootloader version (e.g., 0x00010200 for 1.2.0)
    uint32_t size;              // Size of bootloader binary (excluding header)
    uint32_t crc32;             // CRC32 of bootloader binary
    uint32_t platform;          // Target platform (0x2040 or 0x2350)
    uint32_t min_app_version;   // Minimum compatible app version
    uint32_t flags;             // Update flags (reserved)
    uint32_t header_crc32;      // CRC32 of this header (excluding this field)
} bootloader_update_header_t;

// Update status callback
typedef void (*update_progress_callback_t)(int percent, const char *status);

// Check if a bootloader update file is valid
bootloader_error_t bootloader_update_validate(const char *update_file_path);

// Get information about a bootloader update file
bootloader_error_t bootloader_update_get_info(const char *update_file_path, 
                                              bootloader_update_header_t *header);

// Perform bootloader self-update
// WARNING: This function will reboot the device on success
bootloader_error_t bootloader_update_execute(const char *update_file_path,
                                            update_progress_callback_t progress_cb);

// Check if it's safe to update (battery level, etc.)
bool bootloader_update_safe_to_proceed(void);

// Get current bootloader version
uint32_t bootloader_get_version(void);

#endif // BOOTLOADER_UPDATE_H