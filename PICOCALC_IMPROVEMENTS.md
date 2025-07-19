# PicoCalc SD Bootloader - Improvements & TODO List

This document outlines potential improvements, bug fixes, and feature requests for the PicoCalc SD Bootloader project.

## 🚨 Critical Issues & Bugs

### 1. Hardware Configuration
- **SD Card Pin Conflict**: CS pin is defined as GPIO 17 in config.h, but some references suggest GPIO 22 for card detect. Need to verify actual PicoCalc hardware pinout.
- **Missing Board Definition**: No dedicated `picocalc.h` board definition file in pico-sdk/src/boards/

### 2. Code Quality Issues
- **Hardcoded Values**: Watchdog timeout, buffer sizes, and other constants should be configurable
- **I2C Speed Comment**: Comment says "10kHz for dual i2c" but actual implementation needs verification
- **Error Handling**: Generic error messages need specific error codes and recovery instructions

## 🎯 High Priority Features (Community Requested)

### 1. Auto-Boot Menu System
- Implement GRUB-style boot menu with timeout
- Store default boot selection
- Support for multiple firmware profiles
- Visual countdown timer

### 2. Multi-Stage Boot Process
- Load stage3 bootloader from SD card
- Chain-loading support for different bootloaders
- Dynamic bootloader updates without USB

### 3. Self-Update Capability
- Bootloader update from SD card
- Rollback protection
- Version checking

## 🔧 Medium Priority Enhancements

### 1. Boot Configuration
- `boot.conf` or `picocalc.ini` file support
- Configurable boot parameters
- Default firmware selection
- Timeout settings

### 2. Enhanced UI Features
- Battery percentage display (currently reads but doesn't show)
- Progress bar during flash operations
- Firmware metadata display (version, size, description)
- Subdirectory navigation support
- File sorting options (name, date, size)

### 3. Safety & Reliability
- CRC/hash verification for firmware files
- Dual-boot with automatic fallback
- Firmware backup/restore functionality
- Boot logging to SD card

### 4. Developer Features
- Bootloader API for applications
- Hardware diagnostic mode
- Memory test functionality
- Debug logging system

## 💡 Future Enhancements

### 1. Advanced Features
- Compressed firmware support (.uf2.gz)
- Network boot for Pico W variants
- OTA updates via WiFi
- Secure boot with signature verification

### 2. User Experience
- Search/filter in file browser
- LCD brightness control
- Customizable UI themes
- Multi-language support

### 3. Testing & Quality
- Unit tests for SD card operations
- Hardware-in-the-loop CI/CD tests
- Automated test suite for PicoCalc hardware
- Performance benchmarks

## 📝 Documentation Improvements

### 1. PicoCalc-Specific Docs
- Hardware pinout reference
- Build instructions for PicoCalc
- Troubleshooting guide
- API documentation

### 2. User Guides
- Quick start guide
- Firmware update tutorial
- Recovery procedures
- FAQ section

## 🏗️ Architecture Improvements

### 1. Modularity
- Separate HAL for different hardware variants
- Plugin system for additional features
- Configurable feature flags

### 2. Code Organization
- Move hardcoded values to config files
- Create proper abstraction layers
- Implement dependency injection for testing

## 🐛 Specific Bug Fixes Needed

1. **Flash Operations**
   - Add timeout handling for all flash operations
   - Improve error recovery mechanisms
   - Validate flash boundaries more strictly

2. **SD Card Handling**
   - Better error messages for SD card issues
   - Support for more SD card formats
   - Faster SD card detection

3. **UI Responsiveness**
   - Debounce keyboard input properly
   - Add loading indicators for long operations
   - Prevent UI freezing during flash operations

## 📊 Performance Optimizations

1. **Boot Speed**
   - Optimize SD card initialization
   - Cache frequently accessed data
   - Parallel initialization where possible

2. **Flash Speed**
   - Increase flash programming speed
   - Optimize block size for transfers
   - Implement multi-threaded operations

## 🔒 Security Enhancements

1. **Firmware Validation**
   - Implement cryptographic signatures
   - Add checksum verification
   - Protect against malicious firmware

2. **Access Control**
   - Password protection option
   - Restricted boot modes
   - Audit logging

## 🎨 UI/UX Improvements

1. **Visual Enhancements**
   - Better use of color for status
   - Icon support for file types
   - Smooth scrolling animations

2. **Accessibility**
   - Larger font options
   - High contrast mode
   - Audio feedback support

## 🚀 Getting Started with Improvements

For contributors wanting to help:

1. **High Impact, Easy**: Start with documentation and config file support
2. **Community Value**: Focus on auto-boot menu and configuration features
3. **Technical Depth**: Work on multi-stage boot or self-update mechanisms

## 📅 Suggested Implementation Order

### Phase 1 - Foundation (1-2 weeks)
- Create PicoCalc board definition
- Fix pin configuration issues
- Add basic error codes
- Document PicoCalc build process

### Phase 2 - Core Features (2-4 weeks)
- Implement boot configuration file
- Add auto-boot with timeout
- Create progress indicators
- Add CRC verification

### Phase 3 - Advanced Features (4-8 weeks)
- Multi-stage boot support
- Self-update mechanism
- Dual-boot implementation
- Network boot for Pico W

### Phase 4 - Polish (2-4 weeks)
- UI enhancements
- Performance optimizations
- Comprehensive testing
- Documentation updates

## 🤝 Contributing

When implementing these improvements:
1. Follow existing code style
2. Add unit tests where possible
3. Update documentation
4. Test on actual PicoCalc hardware
5. Consider backward compatibility

## 📚 References

- [ClockworkPi Forum](https://forum.clockworkpi.com/t/uf2-loader-release/18479)
- [Original Blog Post](https://hsuanhanlai.com/writting-custom-bootloader-for-RPI-Pico/)
- [PicoCalc Hardware Specs](TBD - needs documentation)