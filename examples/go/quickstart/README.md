# quickstart — Go wake-word demo over a 16 kHz WAV file

Smallest possible Go pipeline: load the `.vxrt`, walk a WAV file in
32 ms chunks, print every wake-word detection. Pure stdlib — no
`go-audio` / `oto` deps, just `encoding/binary` for the WAV header.

## Install

```sh
go get github.com/VoxRT/voxrt-wake-word-linux/go
```

The bundled native `.so` is found via cgo's `${SRCDIR}/lib/...`
rpath — no `LD_LIBRARY_PATH`, no system install. Toolchain
requirement: Go ≥ 1.21 with cgo enabled (the default; the
`CGO_ENABLED=0` build tag will not work).

## Run

```sh
# Convert any audio to 16 kHz mono 16-bit first if needed:
ffmpeg -i hey_assistant.flac -ar 16000 -ac 1 -sample_fmt s16 hey_assistant.wav

go build .
./quickstart voxrt_wake_word.vxrt hey_assistant.wav
```

Expected output:

```
VoxRT wake-word — Go quickstart
  model: voxrt_wake_word.vxrt
  audio: hey_assistant.wav

  duration: 1.50 s

  [wake] frame=132 t=1.320s score=0.9907

  processed 1.50 s of audio in 0.080 s wall-clock
  RTF = 0.0533 (1 wake event(s) emitted)
```

## Live microphone

For real-time streaming, use any audio capture library that yields
`[]int16` frames at 16 kHz mono — `github.com/gordonklaus/portaudio`,
`github.com/cocoonlife/goalsa`, raw ALSA via `golang.org/x/sys/unix`,
etc. — and call `engine.PushPcmI16(chunk)` per chunk. The streaming
API matches the file loop above.

## What this teaches

- `wakeword.OpenFromPath` / `wakeword.OpenFromBytes` — two
  constructors mirroring the other bindings.
- `engine.SetThreshold(0.9)` / `engine.SetCooldownFrames(100)` —
  returns `error` (range checking happens on the Go side).
- `engine.PushPcmI16([]int16) []wakeword.Detection` — zero-copy view
  over the Go slice into the C ABI (cgo handles the pointer cast).
- `Detection.FrameIndex` (`uint64`) / `.TimestampSec` (`float32`) /
  `.Score` (`float32`) — same shape as Python / Kotlin / Swift /
  C ABI.
- `defer engine.Close()` recommended; a finalizer is registered as a
  safety net but explicit Close is the idiomatic Go pattern.

## Where to go next

- C / C++ equivalent over a live microphone: `../../c/alsa-mic-quickstart/`.
- Python equivalent: `../../python/quickstart/quickstart.py`.
- Node.js equivalent: `../../nodejs/quickstart/quickstart.js`.
