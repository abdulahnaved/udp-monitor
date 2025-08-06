# Cross-compilation toolchain template
# Copy this file and modify for your target architecture

set(CMAKE_SYSTEM_NAME Linux)  # or Windows, Darwin, etc.
set(CMAKE_SYSTEM_PROCESSOR your_arch)  # arm, aarch64, x86_64, etc.

# Specify the cross compiler
set(CMAKE_C_COMPILER your-target-gcc)
set(CMAKE_CXX_COMPILER your-target-g++)

# Where is the target environment located
set(CMAKE_FIND_ROOT_PATH /path/to/your/target/sysroot)

# Adjust the default behavior of the FIND_XXX() commands:
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Architecture-specific flags (modify as needed)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=your-arch-flags")

# Installation prefix
set(CMAKE_INSTALL_PREFIX /opt/udp-monitor-${CMAKE_SYSTEM_PROCESSOR})

# Example configurations:
#
# For ARM64 (aarch64):
# set(CMAKE_SYSTEM_PROCESSOR aarch64)
# set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a")
#
# For MIPS:
# set(CMAKE_SYSTEM_PROCESSOR mips)
# set(CMAKE_C_COMPILER mips-linux-gnu-gcc)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=mips32r2")
#
# For x86 (32-bit):
# set(CMAKE_SYSTEM_PROCESSOR i386)
# set(CMAKE_C_COMPILER i686-linux-gnu-gcc)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")