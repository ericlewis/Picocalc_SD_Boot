# GitHub Actions Common Fixes

This document lists common issues and fixes for the GitHub Actions CI/CD pipeline.

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