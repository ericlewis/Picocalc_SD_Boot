#!/usr/bin/env python3
"""
Create bootloader update package (.bup file) for PicoCalc SD Bootloader
"""

import struct
import sys
import os
import argparse

# Bootloader update file magic number
BOOTLOADER_UPDATE_MAGIC = 0x42554C50  # "BULP"

def crc32(data):
    """Calculate CRC32 using the same algorithm as the bootloader"""
    crc32_table = [
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
        0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
        0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
    ]
    
    crc = 0xffffffff
    for byte in data:
        tbl_idx = crc ^ byte
        crc = crc32_table[tbl_idx & 0x0f] ^ (crc >> 4)
        tbl_idx = crc ^ (byte >> 4)
        crc = crc32_table[tbl_idx & 0x0f] ^ (crc >> 4)
    
    return ~crc & 0xffffffff

def create_bootloader_update(input_file, output_file, version, platform):
    """Create a bootloader update package from a UF2 file"""
    
    # Read the bootloader binary
    with open(input_file, 'rb') as f:
        bootloader_data = f.read()
    
    # Parse version string (e.g., "1.3.0" -> 0x00010300)
    version_parts = version.split('.')
    if len(version_parts) != 3:
        raise ValueError("Version must be in format X.Y.Z")
    
    version_int = (int(version_parts[0]) << 16) | (int(version_parts[1]) << 8) | int(version_parts[2])
    
    # Determine platform
    platform_id = 0x2040 if platform == 'rp2040' else 0x2350
    
    # Create header
    header = struct.pack('<IIIIIIII',
        BOOTLOADER_UPDATE_MAGIC,  # magic
        version_int,              # version
        len(bootloader_data),     # size
        crc32(bootloader_data),   # crc32 of data
        platform_id,              # platform
        0x00010000,              # min_app_version (1.0.0)
        0,                       # flags (reserved)
        0                        # header_crc32 (placeholder)
    )
    
    # Calculate header CRC32 (excluding the last field)
    header_crc = crc32(header[:-4])
    
    # Update header with correct CRC32
    header = header[:-4] + struct.pack('<I', header_crc)
    
    # Write output file
    with open(output_file, 'wb') as f:
        f.write(header)
        f.write(bootloader_data)
    
    print(f"Created bootloader update package: {output_file}")
    print(f"  Version: {version} (0x{version_int:08x})")
    print(f"  Platform: {platform}")
    print(f"  Size: {len(bootloader_data)} bytes")
    print(f"  CRC32: 0x{crc32(bootloader_data):08x}")

def main():
    parser = argparse.ArgumentParser(description='Create bootloader update package')
    parser.add_argument('input', help='Input UF2 file')
    parser.add_argument('-o', '--output', help='Output .bup file', default=None)
    parser.add_argument('-v', '--version', help='Version (X.Y.Z format)', required=True)
    parser.add_argument('-p', '--platform', choices=['rp2040', 'rp2350'], 
                       default='rp2040', help='Target platform')
    
    args = parser.parse_args()
    
    # Generate output filename if not specified
    if args.output is None:
        base = os.path.splitext(args.input)[0]
        args.output = f"{base}_{args.version}_{args.platform}.bup"
    
    create_bootloader_update(args.input, args.output, args.version, args.platform)

if __name__ == '__main__':
    main()