# Cross-Platform Build Support

This directory contains CMake toolchain files for building the UDP Monitor on different architectures.

## Available Toolchains

- **`arm-linux-gnueabihf.cmake`** - ARM 32-bit (Raspberry Pi, etc.)
- **`cross-compile-template.cmake`** - Template for other architectures

## Usage

### Native Build
```bash
mkdir build && cd build
cmake ..
make
```

### Cross-Compilation for ARM
```bash
mkdir build-arm && cd build-arm
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-linux-gnueabihf.cmake ..
make
```


## Prerequisites

Make sure you have the cross-compilation toolchain installed:

```bash
# For ARM (Ubuntu/Debian)
sudo apt-get install gcc-arm-linux-gnueabihf

# For ARM64
sudo apt-get install gcc-aarch64-linux-gnu

# For other architectures, install the appropriate toolchain
```