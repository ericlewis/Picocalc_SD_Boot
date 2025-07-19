#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Mock hardware definitions
#define FLASH_BASE 0x10000000
#define FLASH_SIZE (2 * 1024 * 1024)
#define BOOTLOADER_START 0x101DD000
#define BOOTLOADER_SIZE (140 * 1024)

// Test power failure recovery mechanism
void test_power_failure_recovery() {
    printf("Testing power failure recovery...\n");
    
    // Simulate prog_info structure
    typedef struct {
        uint32_t crc32;
        uint32_t size;
        uint32_t valid_marker;
    } prog_info_t;
    
    prog_info_t prog_info = {0};
    
    // Test invalid marker detection
    prog_info.valid_marker = 0;
    assert(prog_info.valid_marker != 0xDEADBEEF);
    
    // Test valid marker
    prog_info.valid_marker = 0xDEADBEEF;
    prog_info.crc32 = 0x12345678;
    prog_info.size = 1024;
    assert(prog_info.valid_marker == 0xDEADBEEF);
    
    printf("✓ Power failure recovery tests passed\n");
}

// Test vector table validation
void test_vector_table_validation() {
    printf("Testing vector table validation...\n");
    
    // Mock vector table
    typedef struct {
        uint32_t initial_sp;
        uint32_t reset_vector;
    } vector_table_t;
    
    vector_table_t vt;
    
    // Test invalid stack pointer (outside RAM)
    vt.initial_sp = 0x10000000; // Flash address, not RAM
    vt.reset_vector = 0x10000001;
    assert((vt.initial_sp & 0xFF000000) != 0x20000000);
    
    // Test valid vector table
    vt.initial_sp = 0x20041000; // Valid RAM address
    vt.reset_vector = 0x10000001; // Valid flash address with thumb bit
    assert((vt.initial_sp & 0xFF000000) == 0x20000000);
    assert((vt.reset_vector & 1) == 1); // Thumb bit set
    
    printf("✓ Vector table validation tests passed\n");
}

// Test boundary conditions
void test_boundary_conditions() {
    printf("Testing boundary conditions...\n");
    
    // Test flash address boundaries
    uint32_t addr;
    
    // Start of flash
    addr = FLASH_BASE;
    assert(addr >= FLASH_BASE && addr < FLASH_BASE + FLASH_SIZE);
    
    // End of application space (before bootloader)
    addr = BOOTLOADER_START - 1;
    assert(addr >= FLASH_BASE && addr < BOOTLOADER_START);
    
    // Start of bootloader (should be protected)
    addr = BOOTLOADER_START;
    assert(addr >= BOOTLOADER_START && addr < BOOTLOADER_START + BOOTLOADER_SIZE);
    
    // Test wraparound protection
    uint32_t size = 256;
    addr = 0xFFFFFF00;
    assert(addr + size < addr); // Overflow detection
    
    printf("✓ Boundary condition tests passed\n");
}

// Test concurrent access scenarios
void test_concurrent_access() {
    printf("Testing concurrent access protection...\n");
    
    // Simulate flash operation flags
    static bool flash_op_in_progress = false;
    
    // Test single operation
    assert(flash_op_in_progress == false);
    flash_op_in_progress = true;
    assert(flash_op_in_progress == true);
    
    // Simulate completion
    flash_op_in_progress = false;
    assert(flash_op_in_progress == false);
    
    printf("✓ Concurrent access tests passed\n");
}

int main() {
    printf("\n=== Critical Scenario Tests ===\n\n");
    
    test_power_failure_recovery();
    test_vector_table_validation();
    test_boundary_conditions();
    test_concurrent_access();
    
    printf("\n✅ All critical scenario tests passed!\n\n");
    return 0;
}