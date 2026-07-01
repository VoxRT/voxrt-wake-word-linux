# alsa-mic-quickstart-cpp — C++17 wake-word demo over a live ALSA microphone

The `voxrt-wake-word` SDK ships a **C ABI**; the header is already
`extern "C"`-guarded, so any C++ project can `#include
<voxrt_wake_word.h>` and call it directly. This example shows the
idiomatic C++17 shape most consumers want on top of that:

- `VoxrtWakeWord` — RAII wrapper around `voxrt_wake_word_t*` (move-only,
  destroys the handle in the destructor)
- `AlsaCapture` — RAII wrapper around `snd_pcm_t*`
- `std::signal` + `std::atomic<bool>` for a clean Ctrl-C shutdown
- Exception-based error handling
- `std::vector<std::uint8_t>` for the model bytes, `std::vector<std::int16_t>`
  for the PCM chunk

Same runtime, same detection schema as the C example next door.

## Prerequisites

- Linux aarch64 with an ALSA-visible microphone.
- `g++` 7+ (any C++17 compiler works — GCC 7, Clang 5, MSVC 19.15+).
- `libasound2-dev` (Debian/Ubuntu) or `alsa-lib-devel` (Fedora).
- The `voxrt-wake-word` C tarball extracted somewhere — `/usr/local`
  is the easy default:
  ```sh
  curl -L https://github.com/VoxRT/voxrt-wake-word-linux/releases/download/v0.1.5/voxrt-wake-word-0.1.5-aarch64-linux-gnu.tar.gz \
      | sudo tar -xz -C /usr/local --strip-components=1
  sudo ldconfig
  ```
- The `voxrt_wake_word.vxrt` model file:
  ```sh
  curl -LO https://github.com/VoxRT/voxrt-wake-word-models/releases/download/v0.1.0/voxrt_wake_word.vxrt
  ```

## Build

```sh
make
```

If you extracted the tarball to a custom prefix:

```sh
PKG_CONFIG_PATH=~/voxrt-wake-word/lib/pkgconfig make
```

## Run

```sh
./alsa-mic-quickstart-cpp voxrt_wake_word.vxrt              # default: plughw:0
./alsa-mic-quickstart-cpp voxrt_wake_word.vxrt plughw:2,0   # specific card
```

Expected output (speak "Hey Assistant"):

```
[init] model=voxrt_wake_word.vxrt sdk=0.1.5
[init] threshold=0.90 cooldown_frames=100
[init] alsa device=plughw:0 rate=16000 channels=1 format=S16_LE period=512
[init] listening — Ctrl-C to stop
[wake] frame=1331 t=13.310s score=0.9877
[stop] shutting down
```

## What this teaches

- Wrapping opaque C handles in C++ RAII types with `= delete`-d copy,
  move-only, `noexcept` move constructor.
- Feeding raw ALSA PCM into the wake-word engine via the same
  `push_pcm_i16` C entry-point the C example uses.
- Clean async-signal-safe shutdown with `std::atomic<bool>` +
  `extern "C"` signal handler.

## Where to go next

- The plain-C version: `../alsa-mic-quickstart/`.
- A CMake `find_package(VoxRTWakeWord)` skeleton: `../cmake-consumer/`
  (works from C++ unchanged — just set `LANGUAGES CXX` and use `.cpp`).
