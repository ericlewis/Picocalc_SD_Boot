/**
 * PicoCalc SD Firmware Loader - Auto-boot Menu Implementation
 */

#include "auto_boot.h"
#include "text_directory_ui.h"
#include "key_event.h"
#include "../libs/lcdspi/lcdspi.h"
#include "../libs/i2ckbd/i2ckbd.h"
#include "pico/stdlib.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Default configuration
static boot_config_t g_boot_config = {
    .timeout_ms = AUTO_BOOT_TIMEOUT_MS,
    .default_option = BOOT_LAST_APP,
    .show_countdown = true,
    .default_firmware = ""
};

// Menu layout constants
#define MENU_X 40
#define MENU_Y 60
#define MENU_WIDTH 240
#define MENU_HEIGHT 160
#define MENU_ITEM_HEIGHT 30
#define COUNTDOWN_Y (MENU_Y + MENU_HEIGHT + 20)

// Menu items
static const char* boot_menu_items[] = {
    "Boot Last Application",
    "Select Firmware from SD",
    "Enter USB Boot Mode"
};

// Draw the menu frame
static void draw_menu_frame(void) {
    // Clear screen
    lcd_clear();
    
    // Draw title
    lcd_set_cursor(90, 20);
    lcd_print_string_color("PicoCalc Boot Menu", WHITE, BLACK);
    
    // Draw frame
    draw_rect_spi(MENU_X - 2, MENU_Y - 2, MENU_X + MENU_WIDTH + 2, MENU_Y + MENU_HEIGHT + 2, GRAY);
    draw_rect_spi(MENU_X, MENU_Y, MENU_X + MENU_WIDTH, MENU_Y + MENU_HEIGHT, WHITE);
}

// Draw menu items with selection highlight
static void draw_menu_items(boot_option_t selected) {
    for (int i = 0; i < BOOT_OPTION_COUNT; i++) {
        int y = MENU_Y + 20 + (i * MENU_ITEM_HEIGHT);
        
        if (i == selected) {
            // Draw selection highlight
            // Fill rectangle for selection
            for (int dy = 0; dy < 20; dy++) {
                draw_line_spi(MENU_X + 5, y - 2 + dy, MENU_X + MENU_WIDTH - 5, y - 2 + dy, LITEGRAY);
            }
            lcd_set_cursor(MENU_X + 15, y);
            lcd_print_string_color((char*)boot_menu_items[i], BLACK, LITEGRAY);
        } else {
            lcd_set_cursor(MENU_X + 15, y);
            lcd_print_string_color((char*)boot_menu_items[i], WHITE, BLACK);
        }
    }
}

// Draw countdown timer
static void draw_countdown(int seconds_remaining) {
    char countdown_text[64];
    
    // Clear countdown area
    for (int y = 0; y < 30; y++) {
        draw_line_spi(0, COUNTDOWN_Y - 5 + y, 319, COUNTDOWN_Y - 5 + y, BLACK);
    }
    
    if (seconds_remaining > 0) {
        snprintf(countdown_text, sizeof(countdown_text), 
                 "Auto-boot in %d seconds...", seconds_remaining);
        lcd_set_cursor(65, COUNTDOWN_Y);
        lcd_print_string_color(countdown_text, YELLOW, BLACK);
        
        // Draw progress bar
        int bar_width = (MENU_WIDTH * (g_boot_config.timeout_ms - (seconds_remaining * 1000))) / g_boot_config.timeout_ms;
        // Fill progress bar
        for (int y = 0; y < 5; y++) {
            draw_line_spi(MENU_X, COUNTDOWN_Y + 20 + y, MENU_X + bar_width, COUNTDOWN_Y + 20 + y, GREEN);
        }
        // Draw progress bar frame
        draw_rect_spi(MENU_X, COUNTDOWN_Y + 20, MENU_X + MENU_WIDTH, COUNTDOWN_Y + 25, GRAY);
    }
    
    // Draw instructions
    lcd_set_cursor(45, 280);
    lcd_print_string_color("Press any key to interrupt", LITEGRAY, BLACK);
}

// Load configuration from boot.conf file
bootloader_error_t auto_boot_load_config(boot_config_t *config) {
    FILE *fp = fopen(AUTO_BOOT_CONFIG_FILE, "r");
    if (!fp) {
        // No config file, use defaults
        DEBUG_PRINT("No boot.conf found, using defaults\n");
        return ERR_SUCCESS;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Parse timeout
        if (strncmp(line, "timeout=", 8) == 0) {
            config->timeout_ms = atoi(line + 8) * 1000; // Convert seconds to ms
            if (config->timeout_ms > 30000) config->timeout_ms = 30000; // Max 30 seconds
        }
        // Parse default option
        else if (strncmp(line, "default=", 8) == 0) {
            int opt = atoi(line + 8);
            if (opt >= 0 && opt < BOOT_OPTION_COUNT) {
                config->default_option = (boot_option_t)opt;
            }
        }
        // Parse show countdown
        else if (strncmp(line, "show_countdown=", 15) == 0) {
            config->show_countdown = (line[15] == '1' || line[15] == 'y' || line[15] == 'Y');
        }
        // Parse default firmware
        else if (strncmp(line, "default_firmware=", 17) == 0) {
            strncpy(config->default_firmware, line + 17, sizeof(config->default_firmware) - 1);
            // Remove newline
            char *nl = strchr(config->default_firmware, '\n');
            if (nl) *nl = '\0';
        }
    }
    
    fclose(fp);
    DEBUG_PRINT("Loaded boot config: timeout=%dms, default=%d\n", 
                config->timeout_ms, config->default_option);
    return ERR_SUCCESS;
}

// Initialize auto-boot system
bootloader_error_t auto_boot_init(void) {
    // Try to load configuration from SD card
    auto_boot_load_config(&g_boot_config);
    return ERR_SUCCESS;
}

// Get current configuration
const boot_config_t* auto_boot_get_config(void) {
    return &g_boot_config;
}

// Run the auto-boot menu
boot_option_t auto_boot_menu_run(void) {
    boot_option_t selected = g_boot_config.default_option;
    uint32_t start_time = time_us_64();
    uint32_t last_draw_time = 0;
    int last_seconds = -1;
    bool interrupted = false;
    
    // Clear any pending key events
    while (keypad_get_key() > 0) {
        // Drain keypad buffer
    }
    
    // Draw initial menu
    draw_menu_frame();
    draw_menu_items(selected);
    
    // Main menu loop
    while (true) {
        uint32_t current_time = time_us_64();
        uint32_t elapsed_ms = (current_time - start_time) / 1000;
        
        // Check for timeout (only if not interrupted and timeout is enabled)
        if (!interrupted && g_boot_config.timeout_ms > 0) {
            if (elapsed_ms >= g_boot_config.timeout_ms) {
                // Timeout reached, return default option
                DEBUG_PRINT("Auto-boot timeout reached, selecting default option %d\n", selected);
                return selected;
            }
            
            // Update countdown display
            if (g_boot_config.show_countdown) {
                int seconds_remaining = (g_boot_config.timeout_ms - elapsed_ms) / 1000;
                if (seconds_remaining != last_seconds || current_time - last_draw_time > 100000) {
                    draw_countdown(seconds_remaining);
                    last_seconds = seconds_remaining;
                    last_draw_time = current_time;
                }
            }
        } else if (interrupted) {
            // Clear countdown area once interrupted
            if (last_seconds != -1) {
                draw_countdown(0);
                last_seconds = -1;
            }
        }
        
        // Check for key input
        int key = keypad_get_key();
        if (key != 0) {
            interrupted = true; // Any key interrupts auto-boot
            
            switch (key) {
                case KEY_ARROW_UP:
                    if (selected > 0) {
                        selected--;
                        draw_menu_items(selected);
                    }
                    break;
                    
                case KEY_ARROW_DOWN:
                    if (selected < BOOT_OPTION_COUNT - 1) {
                        selected++;
                        draw_menu_items(selected);
                    }
                    break;
                    
                case '\n':  // Enter key
                case KEY_ARROW_RIGHT:
                    DEBUG_PRINT("User selected option %d\n", selected);
                    return selected;
                    
                case 0x1B:  // ESC key
                case KEY_ARROW_LEFT:
                    // ESC defaults to last app
                    return BOOT_LAST_APP;
            }
        }
        
        // Small delay to prevent CPU spinning
        sleep_ms(10);
    }
}