#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../bootloader/include/error_codes.h"

// Mock implementations
void sleep_ms(int ms) { (void)ms; }

// Simple mock for display_error
static char last_error_msg[256] = {0};
static bootloader_error_t last_error_code = ERR_SUCCESS;

const char* get_error_message(bootloader_error_t error);
const char* get_recovery_text(bootloader_error_t error);
recovery_action_t get_recovery_action(bootloader_error_t error);

void text_directory_ui_set_status(const char *msg) {
    strncpy(last_error_msg, msg, sizeof(last_error_msg) - 1);
}

void display_error(bootloader_error_t error) {
    last_error_code = error;
    char msg[256];
    snprintf(msg, sizeof(msg), "Error %d: %s", error, get_error_message(error));
    text_directory_ui_set_status(msg);
}

// Include the actual implementation
#include "../bootloader/src/error_handler.c"

// Test functions
void test_error_messages() {
    printf("Testing error messages...\n");
    
    // Test some specific error codes
    assert(strcmp(get_error_message(ERR_SUCCESS), "Success") == 0);
    assert(strcmp(get_error_message(ERR_SD_NO_CARD), "SD card not detected") == 0);
    assert(strcmp(get_error_message(ERR_UF2_INVALID_MAGIC), "Invalid UF2 file format") == 0);
    assert(strcmp(get_error_message(ERR_FLASH_BOOTLOADER_OVERWRITE), "Attempted bootloader overwrite") == 0);
    assert(strcmp(get_error_message(ERR_UNKNOWN), "Unknown error occurred") == 0);
    
    printf("✓ Error messages test passed\n");
}

void test_recovery_actions() {
    printf("Testing recovery actions...\n");
    
    assert(get_recovery_action(ERR_SUCCESS) == RECOVERY_NONE);
    assert(get_recovery_action(ERR_SD_NO_CARD) == RECOVERY_WAIT_SD_INSERT);
    assert(get_recovery_action(ERR_SD_MOUNT_FAILED) == RECOVERY_FORMAT_SD);
    assert(get_recovery_action(ERR_FLASH_ERASE_FAILED) == RECOVERY_REBOOT);
    
    printf("✓ Recovery actions test passed\n");
}

void test_recovery_text() {
    printf("Testing recovery text...\n");
    
    assert(strcmp(get_recovery_text(ERR_SD_NO_CARD), "Insert SD card and press any key") == 0);
    assert(strcmp(get_recovery_text(ERR_SD_MOUNT_FAILED), "Press F to format or R to retry") == 0);
    assert(strcmp(get_recovery_text(ERR_UF2_CRC_MISMATCH), "Download file again") == 0);
    
    printf("✓ Recovery text test passed\n");
}

void test_display_error() {
    printf("Testing display_error function...\n");
    
    display_error(ERR_SD_NO_CARD);
    assert(last_error_code == ERR_SD_NO_CARD);
    assert(strstr(last_error_msg, "Error 1") != NULL);
    assert(strstr(last_error_msg, "SD card not detected") != NULL);
    
    printf("✓ Display error test passed\n");
}

void test_error_code_uniqueness() {
    printf("Testing error code uniqueness...\n");
    
    // Check that each error code maps to a unique message
    int found_codes[256] = {0};
    
    for (int i = 0; i < sizeof(error_table) / sizeof(error_table[0]); i++) {
        bootloader_error_t code = error_table[i].code;
        assert(code >= 0 && code < 256);
        assert(found_codes[code] == 0); // Ensure no duplicate codes
        found_codes[code] = 1;
    }
    
    printf("✓ Error code uniqueness test passed\n");
}

int main() {
    printf("Running error code tests...\n\n");
    
    test_error_messages();
    test_recovery_actions();
    test_recovery_text();
    test_display_error();
    test_error_code_uniqueness();
    
    printf("\n✅ All tests passed!\n");
    return 0;
}