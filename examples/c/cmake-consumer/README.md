# cmake-consumer — drop voxrt-wake-word into an existing CMake project

Minimum-viable CMake build that consumes the SDK via the standard
`find_package` flow. Use this as the skeleton when you want to wire
the wake-word SDK into a larger CMake project (a robot stack,
a smart-speaker app, a kiosk, a security camera).

This sample doesn't do inference — its job is to prove that
`find_package(VoxRTWakeWord ...)` resolves correctly. For a working
demo with live audio, see `../alsa-mic-quickstart/`.

## Prerequisites

- CMake ≥ 3.16, a C11 compiler, Linux aarch64.
- The `voxrt-wake-word` tarball extracted somewhere:
  ```sh
  curl -L https://github.com/VoxRT/voxrt-wake-word-linux/releases/download/v0.1.5/voxrt-wake-word-0.1.5-aarch64-linux-gnu.tar.gz \
      | sudo tar -xz -C /usr/local --strip-components=1
  ```
  Or into a custom prefix — see "Custom prefix" below.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

```sh
./build/cmake-consumer                                       # version-only
./build/cmake-consumer voxrt_wake_word.vxrt                  # full smoke
```

Expected output:

```
VoxRT wake-word SDK version: 0.1.0
                ABI version: 1.0
    Loaded model: voxrt_wake_word.vxrt (4234560 bytes) — handle 0x55a8...
    OK.
```

## Custom prefix

If the tarball wasn't extracted to `/usr/local`:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=~/voxrt-wake-word
```

`find_package(VoxRTWakeWord …)` will walk down
`<prefix>/lib/cmake/VoxRT/VoxRTWakeWordConfig.cmake` automatically.

## What this teaches

- The shipped CMake config file exposes one imported target,
  `VoxRT::WakeWord`, with the header dir + library path baked in.
- Linking with `target_link_libraries(<your-target> PRIVATE
  VoxRT::WakeWord)` is all you need — no manual `find_library` /
  `include_directories` calls.
- The version check (`find_package(VoxRTWakeWord 0.1 REQUIRED)`)
  accepts any 0.1.x patch release and rejects a 0.2.x / 1.x SDK,
  matching the ABI compatibility promise in `LICENSE-BINARY` /
  `LICENSE`.
