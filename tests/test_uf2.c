#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "../bootloader/include/uf2.h"

// Mock implementations for testing
void sleep_ms(int ms) { (void)ms; }
void text_directory_ui_set_status(const char *msg) { (void)msg; }

// Test basic UF2 block validation
void test_uf2_block_validation() {
    printf("Testing UF2 block validation...\n");
    
    uf2_block_t block = {0};
    
    // Test invalid magic numbers
    block.magic_start0 = 0;
    block.magic_start1 = 0;
    block.magic_end = 0;
    assert(validate_uf2_block(&block) == false);
    
    // Test valid magic numbers
    block.magic_start0 = UF2_MAGIC_START0;
    block.magic_start1 = UF2_MAGIC_START1;
    block.magic_end = UF2_MAGIC_END;
    block.payload_size = 256;
    block.block_no = 0;
    block.num_blocks = 1;
    block.file_size = 256;
    block.target_addr = 0x10000000;
    assert(validate_uf2_block(&block) == true);
    
    // Test invalid payload size
    block.payload_size = 257;
    assert(validate_uf2_block(&block) == false);
    
    block.payload_size = 256;
    
    // Test invalid block number
    block.block_no = 1;
    block.num_blocks = 1;
    assert(validate_uf2_block(&block) == false);
    
    printf("✓ UF2 block validation tests passed\n");
}

// Test bootloader boundary protection
void test_bootloader_protection() {
    printf("Testing bootloader boundary protection...\n");
    
    uf2_block_t block = {
        .magic_start0 = UF2_MAGIC_START0,
        .magic_start1 = UF2_MAGIC_START1,
        .magic_end = UF2_MAGIC_END,
        .payload_size = 256,
        .block_no = 0,
        .num_blocks = 1,
        .file_size = 256
    };
    
    // Test writing to bootloader region (should fail)
    block.target_addr = 0x101DD000; // Inside bootloader region
    assert(is_safe_flash_region(block.target_addr, block.payload_size) == false);
    
    // Test writing to valid application region (should pass)
    block.target_addr = 0x10000000;
    assert(is_safe_flash_region(block.target_addr, block.payload_size) == true);
    
    // Test writing that would overflow into bootloader
    block.target_addr = 0x101DC000;
    block.payload_size = 256;
    assert(is_safe_flash_region(block.target_addr, 0x2000) == false);
    
    printf("✓ Bootloader protection tests passed\n");
}

// Test family ID validation
void test_family_id_validation() {
    printf("Testing family ID validation...\n");
    
    uf2_block_t block = {
        .magic_start0 = UF2_MAGIC_START0,
        .magic_start1 = UF2_MAGIC_START1,
        .magic_end = UF2_MAGIC_END,
        .payload_size = 256,
        .block_no = 0,
        .num_blocks = 1,
        .file_size = 256,
        .target_addr = 0x10000000
    };
    
    // Test RP2040 family ID
    block.family_id = UF2_FAMILY_ID_RP2040;
    assert(validate_uf2_block(&block) == true);
    
    // Test unsupported family ID
    block.family_id = 0x12345678;
    assert(validate_uf2_block(&block) == false);
    
    printf("✓ Family ID validation tests passed\n");
}

int main() {
    printf("\n=== UF2 Implementation Tests ===\n\n");
    
    test_uf2_block_validation();
    test_bootloader_protection();
    test_family_id_validation();
    
    printf("\n✅ All UF2 tests passed!\n\n");
    return 0;
}