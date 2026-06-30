# Minimal CMake project consuming the voxrt-wake-word C SDK.
#
# Demonstrates the standard `find_package` flow — same pattern as
# any other CMake-distributed C library (SDL2, OpenSSL, Protobuf).
#
# Build:
#   cmake -S . -B build
#   cmake --build build
#
# If the tarball was extracted to a custom prefix, point CMake at it:
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=~/voxrt-wake-word
#
# Run (smoke-test — just prints SDK version and tries to load the model):
#   ./build/cmake-consumer voxrt_wake_word.vxrt

cmake_minimum_required(VERSION 3.16)
project(cmake_consumer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Pull in the VoxRT wake-word package. The version requirement
# matches what the consumer was tested against — bump to whatever
# tarball you're installing from. The package config file resolves
# `VoxRT::WakeWord` to an IMPORTED SHARED target that already carries
# the include directory and linker path.
find_package(VoxRTWakeWord 0.1 REQUIRED)

add_executable(cmake-consumer main.c)
target_link_libraries(cmake-consumer PRIVATE VoxRT::WakeWord)
