/**
 * PicoCalc SD Firmware Loader
 *
 * Author: Hsuan Han Lai
 * Email: hsuan.han.lai@gmail.com
 * Website: https://hsuanhanlai.com
 * Year: 2025
 *
 *
 * This project is a bootloader for the PicoCalc device, designed to load and execute
 * firmware applications from an SD card.
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "hardware/sync.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "debug.h"
#include "../libs/i2ckbd/i2ckbd.h"
#include "../libs/lcdspi/lcdspi.h"
#include <hardware/flash.h>
#include <hardware/watchdog.h>
#include "config.h"

#include "blockdevice/sd.h"
#include "filesystem/fat.h"
#include "filesystem/vfs.h"

#include "text_directory_ui.h"
#include "key_event.h"

#include "proginfo.h"
#include "uf2.h"
#include "atu.h"
#include "error_codes.h"
#include "auto_boot.h"


// Vector and RAM offset
#if PICO_RP2040
#define VTOR_OFFSET M0PLUS_VTOR_OFFSET
#define MAX_RAM 0x20040000
#elif PICO_RP2350
#define VTOR_OFFSET M33_VTOR_OFFSET
#define MAX_RAM 0x20080000
#endif

uint8_t status_flag;//0 no sdcard ,1 has sd card
bool sd_card_inserted(void)
{
    status_flag = !gpio_get(SD_DET_PIN);
    // Active low detection - returns true when pin is low
    return (bool)status_flag;
}

bootloader_error_t fs_mount_init() {
    blockdevice_t *sd = blockdevice_sd_create(spi0,
            PICO_DEFAULT_SPI_TX_PIN,  // mosi
            PICO_DEFAULT_SPI_RX_PIN,  // miso
            PICO_DEFAULT_SPI_SCK_PIN, // sclk
            PICO_DEFAULT_SPI_CSN_PIN, // cs
            25 * 1000 * 1000, // 25MHz
            true);

    if (!sd) {
        return ERR_SD_INIT_FAILED;
    }

    filesystem_t *fat = filesystem_fat_create();
    if (!fat) {
        return ERR_OUT_OF_MEMORY;
    }

    int err = fs_mount("/", fat, sd);
    if (err) {
        display_error(ERR_SD_MOUNT_FAILED);
        DEBUG_PRINT("Mount failed with error %d, attempting format...\n", err);
        
        err = fs_format(fat, sd);
        if (err) {
            return ERR_SD_FORMAT_FAILED;
        }
        
        err = fs_mount("/", fat, sd);
        if (err) {
            return ERR_SD_MOUNT_FAILED;
        }
    }
    return ERR_SUCCESS;
}

// This function jumps to the application entry point
// It must update the vector table and stack pointer before jumping
void launch_application_from(uint32_t *app_location)
{
    // https://vanhunteradams.com/Pico/Bootloader/Bootloader.html
    uint32_t *new_vector_table = app_location;
    volatile uint32_t *vtor = (uint32_t *)(PPB_BASE + VTOR_OFFSET);
    *vtor = (uint32_t)new_vector_table;
    asm volatile(
        "msr msp, %0\n"
        "bx %1\n"
        :
        : "r"(new_vector_table[0]), "r"(new_vector_table[1])
        :);
}

int launch_application(void)
{
    if (check_prog_info())
    {
        launch_application_from((void*)get_prog_info()->prog_addr);
    }
}

void boot_fwupdate()
{
    DEBUG_PRINT("entering boot_fwupdate\n");
    lcd_init();
    lcd_clear();

    draw_rect_spi(20, 140, 300, 180, WHITE);
    lcd_set_cursor(30, 150);
    lcd_print_string_color((char *)"FIRMWARE UPDATE", BLACK, WHITE);

    sleep_ms(2000);

    uint gpio_mask = 0u;
    reset_usb_boot(gpio_mask, 0);
}

int load_firmware_by_path(const char *path)
{
    text_directory_ui_set_status("Loading app...");

    // Attempt to load the application from the SD card
    bootloader_error_t load_err = load_application_from_uf2(path);

    if (load_err == ERR_SUCCESS)
    {
        text_directory_ui_set_status("Launching app...");
        DEBUG_PRINT("launching app\n");
        // Small delay to allow printf to complete
        sleep_ms(100);
        launch_application();
    }
    else
    {
        display_error(load_err);
        DEBUG_PRINT("Failed to load app: %s\n", get_error_message(load_err));

        sleep_ms(3000);

        // Trigger a watchdog reboot
        watchdog_reboot(0, 0, 0);
    }
}

void final_selection_callback(const char *path)
{

    char status_message[128];
    const char *extension = ".uf2";

    if (path == NULL)
    {
        // Run current app
        launch_application();
        return;
    }

    // Trigger firmware loading with the selected path
    DEBUG_PRINT("selected: %s\n", path);

    size_t path_len = strlen(path);
    size_t ext_len = strlen(extension);

    if (path_len < ext_len || strcmp(path + path_len - ext_len, extension) != 0)
    {
        DEBUG_PRINT("not a uf2: %s\n", path);
        display_error(ERR_UF2_INVALID_MAGIC);  // Using this as "wrong file type" error
        return;
    }

    snprintf(status_message, sizeof(status_message), "SEL: %s", path);
    text_directory_ui_set_status(status_message);

    sleep_ms(200);

    load_firmware_by_path(path);
}

int read_bootmode()
{
    int key = keypad_get_key();
    int _x;
    DEBUG_PRINT("read_bootmode key = %d\n", key);
    while((_x = keypad_get_key()) > 0) {
        // drain the keypad input buffer
        DEBUG_PRINT("read_bootmode subsequent key = %d\n", _x);
    }
    return key;
}

int main()
{
    stdio_init_all();

#if PICO_RP2350
    // Initialize ATU for application remapping
    if (!atu_init_app_remap()) {
        DEBUG_PRINT("ATU remap alignment error\n");
        // Fallback: reboot to USB boot mode
        uint gpio_mask = 0u;
        reset_usb_boot(gpio_mask, 0);
    }
#endif

    uart_init(uart0, 115200);
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE); // 8-N-1
    uart_set_fifo_enabled(uart0, false);

    // Initialize SD card detection pin
    gpio_init(SD_DET_PIN);
    gpio_set_dir(SD_DET_PIN, GPIO_IN);
    gpio_pull_up(SD_DET_PIN); // Enable pull-up resistor

    keypad_init();
    lcd_init();
    lcd_clear();

    // Check for recovery mode: If prog_info indicates incomplete flash, recover
    volatile prog_info_t const *prog_info = get_prog_info();
    if (prog_info->prog_addr != 0 && prog_info->size == 0xFFFFFFFF) {
        // Size of 0xFFFFFFFF indicates incomplete flash operation
        DEBUG_PRINT("Incomplete flash detected, entering recovery mode\n");
        clear_prog_info();
        // Force SD card boot menu
        goto sd_boot_menu;
    }

    // Initialize auto-boot system (loads config if SD card available)
    if (sd_card_inserted()) {
        bootloader_error_t fs_err = fs_mount_init();
        if (fs_err == ERR_SUCCESS) {
            auto_boot_init();
        }
    }

    // Run auto-boot menu
    boot_option_t boot_choice = auto_boot_menu_run();
    
    DEBUG_PRINT("Boot choice: %d\n", boot_choice);
    
    switch(boot_choice) {
        case BOOT_LAST_APP:
            launch_application();
            break;
            
        case BOOT_USB_MODE:
            boot_fwupdate();
            break;
            
        case BOOT_SD_FIRMWARE:
            // Continue to SD card boot
            break;
    }

sd_boot_menu:

    // BEGIN SDCARD BOOT

    lcd_init();
    lcd_clear();
	text_directory_ui_pre_init();

    // Check for SD card presence
    DEBUG_PRINT("Checking for SD card...\n");
    if (!sd_card_inserted())
    {
        DEBUG_PRINT("SD card not detected\n");
        text_directory_ui_set_status("No SD card found, please insert");

        // Poll until SD card is inserted
        while (!sd_card_inserted())
        {
            sleep_ms(100);
        }
    }

    // Card detected, wait for it to stabilize
    DEBUG_PRINT("SD card detected\n");
    text_directory_ui_set_status("Mounting SD card...");
    sleep_ms(1500); // Wait for card to stabilize

    // Initialize filesystem
    bootloader_error_t fs_err = fs_mount_init();
    if (fs_err != ERR_SUCCESS) {
        handle_error(fs_err);
        // If we get here, error wasn't automatically handled
        sleep_ms(3000);
        watchdog_reboot(0, 0, 0);
    }

    sleep_ms(500);
    lcd_clear();

    text_directory_ui_init();
    text_directory_ui_set_final_callback(final_selection_callback);

    while(keypad_get_key() > 0) {
        // drain the keypad input buffer
    }

    text_directory_ui_run();
}
