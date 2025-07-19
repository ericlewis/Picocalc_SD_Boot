#!/usr/bin/env python3
"""
Validate that the Renode test setup is correctly configured.
This script checks that all required files exist for the tests to run.
"""

import os
import sys

def check_file(path, description):
    """Check if a file exists and report status"""
    exists = os.path.exists(path)
    status = "✓" if exists else "✗"
    print(f"{status} {description}: {path}")
    return exists

def main():
    print("Validating Renode Test Setup")
    print("=" * 50)
    
    # Track if all checks pass
    all_good = True
    
    # Check platform description files
    print("\nPlatform Description Files:")
    platform_files = [
        ("tests/platforms/rp2040_basic.repl", "RP2040 platform description"),
        ("tests/platforms/rp2350_basic.repl", "RP2350 platform description")
    ]
    
    for path, desc in platform_files:
        if not check_file(path, desc):
            all_good = False
    
    # Check test scripts
    print("\nTest Scripts:")
    test_scripts = [
        ("tests/rp2040.resc", "Main RP2040 test script"),
        ("tests/rp2040_simplified.resc", "Simplified test script"),
        ("tests/simple_test.resc", "Simple test script"),
        ("tests/bootloader.robot", "Robot Framework test suite")
    ]
    
    for path, desc in test_scripts:
        if not check_file(path, desc):
            all_good = False
    
    # Check bootloader binaries
    print("\nBootloader Binaries:")
    binary_locations = [
        # Check both possible locations
        ("build/picocalc_sd_boot.elf", "Bootloader ELF (build/)"),
        ("build/bootloader/picocalc_sd_boot.elf", "Bootloader ELF (build/bootloader/)"),
        ("build/picocalc_sd_boot.bin", "Bootloader BIN (build/)"),
        ("build/bootloader/picocalc_sd_boot.bin", "Bootloader BIN (build/bootloader/)"),
        ("build/picocalc_sd_boot.uf2", "Bootloader UF2 (build/)"),
        ("build/bootloader/picocalc_sd_boot.uf2", "Bootloader UF2 (build/bootloader/)")
    ]
    
    has_binaries = False
    for path, desc in binary_locations:
        if check_file(path, desc):
            has_binaries = True
    
    if not has_binaries:
        all_good = False
        print("\n⚠️  No bootloader binaries found. Run build first.")
    
    # Check Python test scripts
    print("\nPython Test Scripts:")
    python_tests = [
        ("tests/run_renode_tests.py", "Main test runner"),
        ("tests/simple_renode_test.py", "Simple test script"),
        ("tests/renode_flash_test.py", "Flash test script"),
        ("tests/renode_test.py", "Renode test helper")
    ]
    
    for path, desc in python_tests:
        if not check_file(path, desc):
            all_good = False
    
    # Summary
    print("\n" + "=" * 50)
    if all_good:
        print("✅ All required files are present!")
        print("\nThe Renode tests should now work when run in the CI environment.")
        print("The tests expect Renode to be installed and available in the PATH.")
    else:
        print("❌ Some required files are missing.")
        print("\nPlease ensure all files are present before running tests.")
        return 1
    
    # Additional notes
    print("\nNotes:")
    print("- Platform files now use generic Renode peripherals")
    print("- Tests no longer require external Python device scripts") 
    print("- Bootloader binaries can be in either build/ or build/bootloader/")
    
    return 0

if __name__ == "__main__":
    sys.exit(main()) 