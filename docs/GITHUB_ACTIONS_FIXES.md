# GitHub Actions Common Fixes

This document lists common issues and fixes for the GitHub Actions CI/CD pipeline.

## Build Artifacts in Wrong Directory

### Problem
The workflow looks for build artifacts in `build/bootloader/` but they're actually in `build/`:
```
ERROR: Could not open 'picocalc_sd_boot.elf'
```

### Solution
The executable is defined in the root `CMakeLists.txt`, not in a subdirectory, so all build artifacts are in the `build/` directory. Update all paths from `build/bootloader/` to `build/`:

```yaml
- name: Create UF2
  run: |
    cd build  # Not build/bootloader
    picotool uf2 convert picocalc_sd_boot.elf picocalc_sd_boot.uf2
```

This affects:
- UF2 creation
- UF2 validation 
- Binary size checking
- Artifact upload/download paths

## RP2350 Build Errors

### Problem
Compilation fails for RP2350 boards (like pico2_w) with errors about conflicting types and undefined structures:
```
error: conflicting types for 'save_and_disable_interrupts'
error: subscripted value is neither array nor pointer nor vector
```

### Solution
1. **Remove hardcoded SDK include paths** from `CMakeLists.txt`. The SDK automatically includes the correct platform-specific headers:
   ```cmake
   # Remove these hardcoded paths:
   ${PICO_SDK_PATH}/src/rp2040/boot_stage2/include
   ${PICO_SDK_PATH}/src/rp2040/hardware_regs/include
   ${PICO_SDK_PATH}/src/rp2040/hardware_structs/include
   ```

2. **Fix include order** in source files. Include `hardware/sync.h` before `pico/bootrom.h`:
   ```c
   #include "hardware/sync.h"  // Must come first
   #include "pico/bootrom.h"
   ```

3. **Make platform-specific includes conditional**. Some headers only exist for RP2040:
   ```c
   #ifndef PICO_RP2350
   #include "hardware/regs/m0plus.h"  // M0+ is RP2040 only
   #endif
   ```

This allows the project to build for both RP2040 and RP2350 platforms.

## Picotool Build Failure: PICO_SDK_PATH not defined

### Problem
When building picotool from source in GitHub Actions, you may see:
```
CMake Error at CMakeLists.txt:14 (message):
  PICO_SDK_PATH is not defined
```

### Solution
Set the `PICO_SDK_PATH` environment variable before building picotool:

```yaml
- name: Install dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y python3-pip ninja-build libusb-1.0-0-dev pkg-config
    # Build picotool from source
    rm -rf picotool
    git clone https://github.com/raspberrypi/picotool.git
    cd picotool
    mkdir build
    cd build
    # Set PICO_SDK_PATH for picotool build
    export PICO_SDK_PATH=${{ github.workspace }}/pico-sdk
    cmake ..
    make
    sudo make install
```

### Recommended Alternative: Automatic Picotool Download

Instead of manually building picotool, let the Pico SDK handle it automatically:

```yaml
- name: Configure
  run: |
    cmake -B build -G Ninja \
      -DPICO_BOARD=${{ matrix.board }} \
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
      -DPICO_SDK_PATH=${PWD}/pico-sdk \
      -DPICOTOOL_FETCH_FROM_GIT=ON
```

This approach is cleaner and ensures version compatibility between the SDK and picotool.

### Requirements
- Ensure the pico-sdk submodule is checked out:
  ```yaml
  - uses: actions/checkout@v4
    with:
      submodules: recursive
  ```

## Act Container Reuse Issues

### Problem
When using act locally, you may see "directory already exists" errors due to container reuse.

### Solution
Clean up directories before operations:
```yaml
# Remove directory if it exists
rm -rf picotool
git clone https://github.com/raspberrypi/picotool.git
```

## Architecture Mismatches on Apple Silicon

### Problem
x86_64 binaries fail to run on ARM64 runners with Rosetta errors.

### Solution
1. Use Docker-based testing for Apple Silicon:
   ```bash
   ./tools/run_ci_locally_docker.sh
   ```

2. Or configure act for ARM64:
   ```bash
   act --container-architecture linux/arm64
   ```

## External Actions Authentication

### Problem
Act fails to download external GitHub Actions without authentication.

### Solution
Set GitHub token:
```bash
export GITHUB_TOKEN=$(gh auth token)
act --secret GITHUB_TOKEN=$GITHUB_TOKEN
``` 