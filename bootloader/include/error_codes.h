#ifndef ERROR_CODES_H
#define ERROR_CODES_H

// Error code definitions for PicoCalc bootloader
typedef enum {
    // Success
    ERR_SUCCESS = 0,
    
    // SD Card errors (1-20)
    ERR_SD_NO_CARD = 1,
    ERR_SD_MOUNT_FAILED = 2,
    ERR_SD_FORMAT_FAILED = 3,
    ERR_SD_READ_FAILED = 4,
    ERR_SD_WRITE_FAILED = 5,
    ERR_SD_INIT_FAILED = 6,
    ERR_SD_INVALID_FS = 7,
    
    // UF2 errors (21-40)
    ERR_UF2_INVALID_MAGIC = 21,
    ERR_UF2_INVALID_SIZE = 22,
    ERR_UF2_INVALID_ADDR = 23,
    ERR_UF2_INVALID_FAMILY = 24,
    ERR_UF2_CRC_MISMATCH = 25,
    ERR_UF2_VERIFY_FAILED = 26,
    ERR_UF2_FILE_NOT_FOUND = 27,
    ERR_UF2_FILE_TOO_LARGE = 28,
    
    // Flash errors (41-60)
    ERR_FLASH_ERASE_FAILED = 41,
    ERR_FLASH_WRITE_FAILED = 42,
    ERR_FLASH_VERIFY_FAILED = 43,
    ERR_FLASH_BOUNDARY_VIOLATION = 44,
    ERR_FLASH_BOOTLOADER_OVERWRITE = 45,
    ERR_FLASH_BOOT2_OVERWRITE = 46,
    ERR_FLASH_TIMEOUT = 47,
    
    // System errors (61-80)
    ERR_OUT_OF_MEMORY = 61,
    ERR_INVALID_PARAMETER = 62,
    ERR_NOT_INITIALIZED = 63,
    ERR_WATCHDOG_TIMEOUT = 64,
    ERR_VECTOR_TABLE_INVALID = 65,
    ERR_STACK_OVERFLOW = 66,
    
    // Hardware errors (81-100)
    ERR_LCD_INIT_FAILED = 81,
    ERR_KEYBOARD_INIT_FAILED = 82,
    ERR_I2C_COMM_FAILED = 83,
    ERR_SPI_COMM_FAILED = 84,
    ERR_GPIO_INIT_FAILED = 85,
    
    // File system errors (101-120)
    ERR_FS_DIR_NOT_FOUND = 101,
    ERR_FS_FILE_NOT_FOUND = 102,
    ERR_FS_PATH_TOO_LONG = 103,
    ERR_FS_ACCESS_DENIED = 104,
    ERR_FS_CORRUPTED = 105,
    
    // Unknown error
    ERR_UNKNOWN = 255
} bootloader_error_t;

// Error recovery actions
typedef enum {
    RECOVERY_NONE = 0,
    RECOVERY_RETRY,
    RECOVERY_REBOOT,
    RECOVERY_FORMAT_SD,
    RECOVERY_ENTER_BOOTSEL,
    RECOVERY_WAIT_SD_INSERT,
    RECOVERY_CHECK_CONNECTIONS,
    RECOVERY_POWER_CYCLE
} recovery_action_t;

// Error information structure
typedef struct {
    bootloader_error_t code;
    const char* message;
    recovery_action_t recovery;
    const char* recovery_text;
} error_info_t;

// Function prototypes
const char* get_error_message(bootloader_error_t error);
recovery_action_t get_recovery_action(bootloader_error_t error);
const char* get_recovery_text(bootloader_error_t error);
void display_error(bootloader_error_t error);
void handle_error(bootloader_error_t error);

#endif // ERROR_CODES_H