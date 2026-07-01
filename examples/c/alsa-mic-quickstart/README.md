# alsa-mic-quickstart — C wake-word demo over a live ALSA microphone

Minimum-viable C example: open the microphone, pump 32 ms chunks
into `voxrt_wake_word_session_push_pcm_i16`, print every detection
on stdout.

Same algorithm + output schema as the Rust `score_alsa` example
that ships with the wake-word crate.

## Prerequisites

- Linux aarch64 with an ALSA-visible microphone (USB mic, I²S board,
  SPH0645, MEMS hat, etc.).
- `libasound2-dev` (Debian/Ubuntu) or `alsa-lib-devel` (Fedora).
- The `voxrt-wake-word` C tarball extracted somewhere — `/usr/local`
  is the easy default, but any prefix works:
  ```sh
  curl -L https://github.com/VoxRT/voxrt-wake-word-linux/releases/download/v0.1.3/voxrt-wake-word-0.1.3-aarch64-linux-gnu.tar.gz \
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
./alsa-mic-quickstart voxrt_wake_word.vxrt              # default: plughw:0
./alsa-mic-quickstart voxrt_wake_word.vxrt plughw:2,0   # specific card
```

Expected output (speak "Hey Assistant"):

```
[init] model=voxrt_wake_word.vxrt sdk=0.1.0
[init] threshold=0.90 cooldown_frames=100
[init] alsa device=plughw:0 rate=16000 channels=1 format=S16_LE period=512
[init] listening — Ctrl-C to stop
[wake] frame=1331 t=13.310s score=0.9877
```

## What this teaches

- Loading a `.vxrt` via `mmap` and passing the bytes to
  `voxrt_wake_word_create`.
- Configuring threshold + cooldown.
- Streaming i16 PCM via `voxrt_wake_word_push_pcm_i16` and printing
  the per-call detection batch.
- ALSA xrun recovery (`snd_pcm_prepare` on EPIPE).

## Where to go next

- For a "drop into a CMake project" pattern instead of a single-file
  build, see `../cmake-consumer/`.
- For per-second RTF stats on top of detections, look at the Rust
  `score_alsa` source — porting that block to C is ~30 lines.
