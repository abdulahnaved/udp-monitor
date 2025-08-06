# Cross-compilation toolchain for ARM Linux (like Raspberry Pi)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# Where is the target environment located
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)

# Adjust the default behavior of the FIND_XXX() commands:
# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Additional ARM-specific flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard")

# Installation prefix for ARM binaries
set(CMAKE_INSTALL_PREFIX /opt/udp-monitor-arm)