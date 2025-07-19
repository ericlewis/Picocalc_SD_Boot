/**
 * PicoCalc SD Firmware Loader - Bootloader Self-Update Implementation
 * 
 * CRITICAL: This code updates the bootloader itself. Extreme care must be taken
 * to avoid bricking the device. The update process runs entirely from RAM.
 */

#include "bootloader_update.h"
#include "bldetect.h"
#include "proginfo.h"
#include "error_codes.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <hardware/watchdog.h>
#include <pico/bootrom.h>
#include "pico/stdlib.h"
#include "../libs/lcdspi/lcdspi.h"
#include "text_directory_ui.h"

// Current bootloader version (1.3.0)
#define BOOTLOADER_VERSION 0x00010300

// RAM buffer for update operations (64KB should be enough)
#define UPDATE_BUFFER_SIZE (64 * 1024)
static uint8_t update_buffer[UPDATE_BUFFER_SIZE] __attribute__((aligned(256)));

// CRC32 implementation
static const uint32_t crc32_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

static uint32_t crc32(const uint8_t *data, size_t size) {
    uint32_t crc = 0xffffffff;
    for (size_t i = 0; i < size; i++) {
        int tbl_idx = crc ^ data[i];
        crc = crc32_table[tbl_idx & 0x0f] ^ (crc >> 4);
        tbl_idx = crc ^ (data[i] >> 4);
        crc = crc32_table[tbl_idx & 0x0f] ^ (crc >> 4);
    }
    return ~crc;
}

// Forward declaration for CRC32 update
static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len);

// Get current bootloader version
uint32_t bootloader_get_version(void) {
    return BOOTLOADER_VERSION;
}

// Check if battery level is sufficient for update
bool bootloader_update_safe_to_proceed(void) {
    // TODO: Read actual battery level
    // For now, always return true
    return true;
}

// Validate bootloader update file
bootloader_error_t bootloader_update_validate(const char *update_file_path) {
    FILE *fp = fopen(update_file_path, "rb");
    if (!fp) {
        return ERR_FILE_NOT_FOUND;
    }
    
    bootloader_update_header_t header;
    size_t read = fread(&header, 1, sizeof(header), fp);
    if (read != sizeof(header)) {
        fclose(fp);
        return ERR_FILE_READ_ERROR;
    }
    
    // Check magic number
    if (header.magic != BOOTLOADER_UPDATE_MAGIC) {
        fclose(fp);
        return ERR_UF2_INVALID_MAGIC;
    }
    
    // Verify header CRC32 (excluding the header_crc32 field itself)
    uint32_t calc_crc = crc32((uint8_t*)&header, sizeof(header) - sizeof(uint32_t));
    if (calc_crc != header.header_crc32) {
        fclose(fp);
        return ERR_UF2_CRC_MISMATCH;
    }
    
    // Check platform compatibility
#if PICO_RP2040
    if (header.platform != 0x2040) {
        fclose(fp);
        return ERR_UF2_WRONG_FAMILY;
    }
#elif PICO_RP2350
    if (header.platform != 0x2350) {
        fclose(fp);
        return ERR_UF2_WRONG_FAMILY;
    }
#endif
    
    // Check size constraints
    if (header.size > MAX_BOOTLOADER_SIZE || header.size < 1024) {
        fclose(fp);
        return ERR_UF2_TOO_LARGE;
    }
    
    // Verify bootloader binary CRC32
    uint32_t binary_crc = 0;
    size_t remaining = header.size;
    
    while (remaining > 0) {
        size_t to_read = (remaining > UPDATE_BUFFER_SIZE) ? UPDATE_BUFFER_SIZE : remaining;
        size_t actual = fread(update_buffer, 1, to_read, fp);
        if (actual != to_read) {
            fclose(fp);
            return ERR_FILE_READ_ERROR;
        }
        
        // Update CRC32
        if (binary_crc == 0) {
            binary_crc = crc32(update_buffer, actual);
        } else {
            // Continue CRC calculation
            binary_crc = crc32_update(binary_crc, update_buffer, actual);
        }
        
        remaining -= actual;
    }
    
    fclose(fp);
    
    if (binary_crc != header.crc32) {
        return ERR_UF2_CRC_MISMATCH;
    }
    
    return ERR_SUCCESS;
}

// Get bootloader update info
bootloader_error_t bootloader_update_get_info(const char *update_file_path, 
                                              bootloader_update_header_t *header) {
    if (!header) {
        return ERR_INVALID_PARAMETER;
    }
    
    FILE *fp = fopen(update_file_path, "rb");
    if (!fp) {
        return ERR_FILE_NOT_FOUND;
    }
    
    size_t read = fread(header, 1, sizeof(*header), fp);
    fclose(fp);
    
    if (read != sizeof(*header)) {
        return ERR_FILE_READ_ERROR;
    }
    
    if (header->magic != BOOTLOADER_UPDATE_MAGIC) {
        return ERR_UF2_INVALID_MAGIC;
    }
    
    return ERR_SUCCESS;
}

// Critical function - runs entirely from RAM
static void __no_inline_not_in_flash_func(perform_bootloader_update)(
    const uint8_t *new_bootloader, 
    size_t size,
    uint32_t bootloader_start_addr) {
    
    // Disable interrupts
    uint32_t ints = save_and_disable_interrupts();
    
    // Calculate number of sectors to erase
    uint32_t num_sectors = (size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;
    
    // Erase bootloader area
    flash_range_erase(bootloader_start_addr - XIP_BASE, num_sectors * FLASH_SECTOR_SIZE);
    
    // Program new bootloader
    flash_range_program(bootloader_start_addr - XIP_BASE, new_bootloader, size);
    
    // Re-enable interrupts
    restore_interrupts(ints);
    
    // Force a system reset
    watchdog_enable(1, 1);
    while(1);
}

// Execute bootloader update
bootloader_error_t bootloader_update_execute(const char *update_file_path,
                                            update_progress_callback_t progress_cb) {
    // First validate the update file
    bootloader_error_t err = bootloader_update_validate(update_file_path);
    if (err != ERR_SUCCESS) {
        return err;
    }
    
    // Check if safe to proceed
    if (!bootloader_update_safe_to_proceed()) {
        return ERR_LOW_BATTERY;
    }
    
    // Get bootloader start address
    extern int __logical_binary_start;
    uint32_t bootloader_start = (uint32_t)&__logical_binary_start;
    
    // Verify we're running as a bootloader (high memory)
    if (bootloader_start < XIP_BASE + (1024 * 1024)) {
        // Bootloader should be in high memory
        return ERR_BOOTLOADER_NOT_FOUND;
    }
    
    // Open update file
    FILE *fp = fopen(update_file_path, "rb");
    if (!fp) {
        return ERR_FILE_NOT_FOUND;
    }
    
    // Read header
    bootloader_update_header_t header;
    fread(&header, 1, sizeof(header), fp);
    
    // Allocate memory for entire bootloader
    uint8_t *new_bootloader = malloc(header.size);
    if (!new_bootloader) {
        fclose(fp);
        return ERR_OUT_OF_MEMORY;
    }
    
    if (progress_cb) {
        progress_cb(0, "Reading update file...");
    }
    
    // Read entire bootloader into memory
    size_t total_read = 0;
    while (total_read < header.size) {
        size_t to_read = header.size - total_read;
        if (to_read > UPDATE_BUFFER_SIZE) to_read = UPDATE_BUFFER_SIZE;
        
        size_t actual = fread(new_bootloader + total_read, 1, to_read, fp);
        if (actual != to_read) {
            free(new_bootloader);
            fclose(fp);
            return ERR_FILE_READ_ERROR;
        }
        
        total_read += actual;
        
        if (progress_cb) {
            int percent = (total_read * 50) / header.size;  // 0-50% for reading
            progress_cb(percent, "Reading update file...");
        }
    }
    
    fclose(fp);
    
    // Final safety check - verify CRC32 again
    uint32_t final_crc = crc32(new_bootloader, header.size);
    if (final_crc != header.crc32) {
        free(new_bootloader);
        return ERR_UF2_CRC_MISMATCH;
    }
    
    if (progress_cb) {
        progress_cb(60, "Preparing to update bootloader...");
    }
    
    // Clear any pending watchdog
    watchdog_update();
    
    // Display warning message
    lcd_clear();
    lcd_set_cursor(40, 120);
    lcd_print_string_color("UPDATING BOOTLOADER", WHITE, BLACK);
    lcd_set_cursor(60, 140);
    lcd_print_string_color("DO NOT POWER OFF!", RED, BLACK);
    lcd_set_cursor(50, 180);
    lcd_print_string_color("This will take ~10 seconds", WHITE, BLACK);
    
    if (progress_cb) {
        progress_cb(70, "Updating bootloader...");
    }
    
    // Wait a bit to ensure message is visible
    sleep_ms(2000);
    
    // CRITICAL SECTION - Perform the actual update from RAM
    perform_bootloader_update(new_bootloader, header.size, bootloader_start);
    
    // We should never reach here
    free(new_bootloader);
    return ERR_UNKNOWN;
}

// Helper function to continue CRC32 calculation  
static uint32_t crc32_update(uint32_t crc_in, const uint8_t *data, size_t size) {
    // Invert input CRC to continue calculation
    uint32_t crc = ~crc_in;
    
    for (size_t i = 0; i < size; i++) {
        int tbl_idx = crc ^ data[i];
        crc = crc32_table[tbl_idx & 0x0f] ^ (crc >> 4);
        tbl_idx = crc ^ (data[i] >> 4);
        crc = crc32_table[tbl_idx & 0x0f] ^ (crc >> 4);
    }
    
    return ~crc;
}