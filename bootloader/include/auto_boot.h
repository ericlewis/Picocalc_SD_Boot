/**
 * PicoCalc SD Firmware Loader - Auto-boot Menu System
 * 
 * Implements a GRUB-style auto-boot menu with timeout as requested by the ClockworkPi community
 */

#ifndef AUTO_BOOT_H
#define AUTO_BOOT_H

#include <stdint.h>
#include <stdbool.h>
#include "error_codes.h"

// Auto-boot configuration
#define AUTO_BOOT_TIMEOUT_MS 5000  // 5 seconds default timeout
#define AUTO_BOOT_CONFIG_FILE "/boot.conf"

// Boot options
typedef enum {
    BOOT_LAST_APP = 0,     // Boot the last flashed application
    BOOT_SD_FIRMWARE,      // Boot from SD card firmware directory
    BOOT_USB_MODE,         // Enter USB boot mode
    BOOT_OPTION_COUNT
} boot_option_t;

// Boot configuration structure
typedef struct {
    uint32_t timeout_ms;           // Timeout in milliseconds (0 = no timeout)
    boot_option_t default_option;  // Default boot option
    bool show_countdown;           // Whether to show countdown timer
    char default_firmware[256];    // Default firmware file to load from SD
} boot_config_t;

// Initialize auto-boot system
bootloader_error_t auto_boot_init(void);

// Run the auto-boot menu
// Returns the selected boot option
boot_option_t auto_boot_menu_run(void);

// Load boot configuration from SD card
bootloader_error_t auto_boot_load_config(boot_config_t *config);

// Get the current boot configuration
const boot_config_t* auto_boot_get_config(void);

#endif // AUTO_BOOT_H