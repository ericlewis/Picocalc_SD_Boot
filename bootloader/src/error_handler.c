#include "error_codes.h"
#include "text_directory_ui.h"
#include "debug.h"
#include <stdio.h>
#include <stdint.h>
#include <pico/stdlib.h>
#include <hardware/watchdog.h>
#include <pico/bootrom.h>

// Error information table
static const error_info_t error_table[] = {
    // Success
    {ERR_SUCCESS, "Success", RECOVERY_NONE, ""},
    
    // SD Card errors
    {ERR_SD_NO_CARD, "SD card not detected", RECOVERY_WAIT_SD_INSERT, "Insert SD card and press any key"},
    {ERR_SD_MOUNT_FAILED, "Failed to mount SD card", RECOVERY_FORMAT_SD, "Press F to format or R to retry"},
    {ERR_SD_FORMAT_FAILED, "Failed to format SD card", RECOVERY_CHECK_CONNECTIONS, "Check SD card connections"},
    {ERR_SD_READ_FAILED, "Failed to read from SD card", RECOVERY_RETRY, "Press R to retry"},
    {ERR_SD_WRITE_FAILED, "SD card write failed", RECOVERY_CHECK_CONNECTIONS, "Check if card is write-protected"},
    {ERR_SD_INIT_FAILED, "SD card initialization failed", RECOVERY_POWER_CYCLE, "Remove and reinsert SD card"},
    {ERR_SD_INVALID_FS, "Invalid filesystem on SD card", RECOVERY_FORMAT_SD, "Press F to format"},
    
    // UF2 errors
    {ERR_UF2_INVALID_MAGIC, "Invalid UF2 file format", RECOVERY_NONE, "Select a valid UF2 file"},
    {ERR_UF2_INVALID_SIZE, "UF2 file size invalid", RECOVERY_NONE, "File may be corrupted"},
    {ERR_UF2_INVALID_ADDR, "Invalid flash address in UF2", RECOVERY_NONE, "UF2 not compatible with device"},
    {ERR_UF2_INVALID_FAMILY, "Wrong device family in UF2", RECOVERY_NONE, "Use RP2040/RP2350 compatible file"},
    {ERR_UF2_CRC_MISMATCH, "UF2 data corruption detected", RECOVERY_NONE, "Download file again"},
    {ERR_UF2_VERIFY_FAILED, "Flash verification failed", RECOVERY_RETRY, "Press R to retry flash"},
    {ERR_UF2_FILE_NOT_FOUND, "UF2 file not found", RECOVERY_NONE, "Check file exists on SD card"},
    {ERR_UF2_FILE_TOO_LARGE, "UF2 file too large", RECOVERY_NONE, "File exceeds available flash"},
    {ERR_UF2_WRONG_FAMILY, "Wrong device family", RECOVERY_NONE, "Use correct firmware file"},
    {ERR_UF2_TOO_LARGE, "File too large", RECOVERY_NONE, "File exceeds size limits"},
    
    // Flash errors
    {ERR_FLASH_ERASE_FAILED, "Flash erase failed", RECOVERY_REBOOT, "Press R to reboot"},
    {ERR_FLASH_WRITE_FAILED, "Flash write failed", RECOVERY_REBOOT, "Press R to reboot"},
    {ERR_FLASH_VERIFY_FAILED, "Flash verification failed", RECOVERY_RETRY, "Press R to retry"},
    {ERR_FLASH_BOUNDARY_VIOLATION, "Flash boundary violation", RECOVERY_NONE, "Invalid firmware size"},
    {ERR_FLASH_BOOTLOADER_OVERWRITE, "Attempted bootloader overwrite", RECOVERY_NONE, "Invalid firmware address"},
    {ERR_FLASH_BOOT2_OVERWRITE, "Attempted boot2 overwrite", RECOVERY_NONE, "Invalid firmware address"},
    {ERR_FLASH_TIMEOUT, "Flash operation timeout", RECOVERY_REBOOT, "Press R to reboot"},
    
    // System errors
    {ERR_OUT_OF_MEMORY, "Out of memory", RECOVERY_REBOOT, "Press R to reboot"},
    {ERR_INVALID_PARAMETER, "Invalid parameter", RECOVERY_NONE, "Internal error"},
    {ERR_NOT_INITIALIZED, "System not initialized", RECOVERY_REBOOT, "Press R to reboot"},
    {ERR_WATCHDOG_TIMEOUT, "Watchdog timeout", RECOVERY_REBOOT, "System will reboot"},
    {ERR_VECTOR_TABLE_INVALID, "Invalid vector table", RECOVERY_NONE, "Firmware corrupted"},
    {ERR_STACK_OVERFLOW, "Stack overflow detected", RECOVERY_REBOOT, "Press R to reboot"},
    
    // Hardware errors
    {ERR_LCD_INIT_FAILED, "LCD initialization failed", RECOVERY_CHECK_CONNECTIONS, "Check LCD connections"},
    {ERR_KEYBOARD_INIT_FAILED, "Keyboard init failed", RECOVERY_CHECK_CONNECTIONS, "Check keyboard connections"},
    {ERR_I2C_COMM_FAILED, "I2C communication failed", RECOVERY_CHECK_CONNECTIONS, "Check I2C connections"},
    {ERR_SPI_COMM_FAILED, "SPI communication failed", RECOVERY_CHECK_CONNECTIONS, "Check SPI connections"},
    {ERR_GPIO_INIT_FAILED, "GPIO initialization failed", RECOVERY_REBOOT, "Press R to reboot"},
    {ERR_LOW_BATTERY, "Battery too low", RECOVERY_POWER_CYCLE, "Charge device before updating"},
    
    // File system errors
    {ERR_FS_DIR_NOT_FOUND, "Directory not found", RECOVERY_NONE, "Check path"},
    {ERR_FS_FILE_NOT_FOUND, "File not found", RECOVERY_NONE, "Check filename"},
    {ERR_FS_PATH_TOO_LONG, "Path too long", RECOVERY_NONE, "Use shorter path"},
    {ERR_FS_ACCESS_DENIED, "Access denied", RECOVERY_NONE, "Check permissions"},
    {ERR_FS_CORRUPTED, "Filesystem corrupted", RECOVERY_FORMAT_SD, "Press F to format"},
    {ERR_FILE_NOT_FOUND, "File not found", RECOVERY_NONE, "Check file exists on SD"},
    {ERR_FILE_READ_ERROR, "File read error", RECOVERY_RETRY, "Press R to retry"},
    
    // Bootloader update errors
    {ERR_BOOTLOADER_NOT_FOUND, "Bootloader not detected", RECOVERY_ENTER_BOOTSEL, "Hold BOOTSEL and reset"},
    {ERR_BOOTLOADER_UPDATE_INVALID, "Invalid bootloader update", RECOVERY_NONE, "Check update file integrity"},
    {ERR_BOOTLOADER_UPDATE_WRONG_PLATFORM, "Wrong platform update", RECOVERY_NONE, "Use RP2040/RP2350 specific file"},
    {ERR_BOOTLOADER_UPDATE_TOO_LARGE, "Update file too large", RECOVERY_NONE, "Bootloader exceeds size limit"},
    {ERR_BOOTLOADER_UPDATE_CRC_FAIL, "Update CRC check failed", RECOVERY_RETRY, "Download update file again"},
    {ERR_BOOTLOADER_UPDATE_VERSION_MISMATCH, "Version incompatible", RECOVERY_NONE, "Check version requirements"},
    
    // Unknown error
    {ERR_UNKNOWN, "Unknown error occurred", RECOVERY_REBOOT, "Press R to reboot"}
};

// Get error message for a given error code
const char* get_error_message(bootloader_error_t error) {
    for (size_t i = 0; i < sizeof(error_table) / sizeof(error_table[0]); i++) {
        if (error_table[i].code == error) {
            return error_table[i].message;
        }
    }
    return error_table[sizeof(error_table) / sizeof(error_table[0]) - 1].message; // Unknown error
}

// Get recovery action for a given error code
recovery_action_t get_recovery_action(bootloader_error_t error) {
    for (size_t i = 0; i < sizeof(error_table) / sizeof(error_table[0]); i++) {
        if (error_table[i].code == error) {
            return error_table[i].recovery;
        }
    }
    return RECOVERY_REBOOT;
}

// Get recovery text for a given error code
const char* get_recovery_text(bootloader_error_t error) {
    for (size_t i = 0; i < sizeof(error_table) / sizeof(error_table[0]); i++) {
        if (error_table[i].code == error) {
            return error_table[i].recovery_text;
        }
    }
    return "Press R to reboot";
}

// Display error on LCD with recovery instructions
void display_error(bootloader_error_t error) {
    char error_msg[128];
    
    // Format error message with code
    snprintf(error_msg, sizeof(error_msg), "Error %d: %s", error, get_error_message(error));
    
    // Display on LCD
    text_directory_ui_set_status(error_msg);
    
    // Also display recovery text if available
    const char* recovery = get_recovery_text(error);
    if (recovery && recovery[0] != '\0') {
        // TODO: Add second line status display or show in file area
        DEBUG_PRINT("Recovery: %s\n", recovery);
    }
    
    // Log to debug output
    DEBUG_PRINT("ERROR: %s (code %d)\n", get_error_message(error), error);
    DEBUG_PRINT("Recovery: %s\n", recovery);
}

// Handle error with appropriate recovery action
void handle_error(bootloader_error_t error) {
    // Display the error first
    display_error(error);
    
    // Get recovery action
    recovery_action_t action = get_recovery_action(error);
    
    // Wait for user input or auto-execute recovery
    switch (action) {
        case RECOVERY_NONE:
            // Just display error, no automatic action
            break;
            
        case RECOVERY_RETRY:
            // User needs to press R to retry
            // This should be handled by the calling code
            break;
            
        case RECOVERY_REBOOT:
            // Wait a moment for user to see error
            sleep_ms(3000);
            // Reboot system
            watchdog_enable(1, false);
            while(1);
            break;
            
        case RECOVERY_FORMAT_SD:
            // User needs to press F to format
            // This should be handled by the calling code
            break;
            
        case RECOVERY_ENTER_BOOTSEL:
            // Enter BOOTSEL mode
            sleep_ms(2000);
            reset_usb_boot(0, 0);
            break;
            
        case RECOVERY_WAIT_SD_INSERT:
        case RECOVERY_CHECK_CONNECTIONS:
        case RECOVERY_POWER_CYCLE:
            // These require user action
            // Calling code should handle waiting for user input
            break;
    }
}