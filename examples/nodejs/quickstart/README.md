# quickstart — Node.js wake-word demo over a 16 kHz WAV file

Smallest possible Node.js pipeline: load the `.vxrt`, walk a WAV
file in 32 ms chunks, print every wake-word detection. Pure stdlib —
no `wavefile` / `node-wav` / `mic` dependency, just `fs` and the
`@voxrt/wake-word` package.

## Install

```sh
npm install @voxrt/wake-word
```

Get the wake-phrase model:

```sh
curl -LO https://github.com/VoxRT/voxrt-wake-word-models/releases/download/v0.1.0/voxrt_wake_word.vxrt
```

To pin a specific SDK version's tarball from a GitHub Release:

```sh
curl -LO https://github.com/VoxRT/voxrt-wake-word-linux/releases/download/v0.1.4/voxrt-wake-word-0.1.4.tgz
npm install ./voxrt-wake-word-0.1.4.tgz
```

## Run

```sh
# Convert any audio to 16 kHz mono 16-bit first if needed:
ffmpeg -i hey_assistant.flac -ar 16000 -ac 1 -sample_fmt s16 hey_assistant.wav

node quickstart.js voxrt_wake_word.vxrt hey_assistant.wav
```

Expected output:

```
VoxRT wake-word — Node.js quickstart
  model: voxrt_wake_word.vxrt
  audio: hey_assistant.wav

  duration: 1.50 s

  [wake] frame=132 t=1.320s score=0.9907

  processed 1.50 s of audio in 0.080 s wall-clock
  RTF = 0.0533 (1 wake event(s) emitted)
```

## Live microphone

For real-time streaming, install one of the standard ALSA / Web Audio
bridges (`naudiodon`, `node-microphone`, `mic`, the `microphone`
WebSocket protocol, etc.) and feed `Int16Array` chunks to
`engine.pushPcmI16(...)`. The streaming API is identical to the file
loop here — just swap `wave` reading for your audio source.

## What this teaches

- `WakeWordEngine.fromPath` / `WakeWordEngine.fromBytes` — two
  constructors mirroring every other binding.
- `engine.threshold = 0.9` / `engine.cooldownFrames = 100` — TS-typed
  properties, mutable at any time.
- `engine.pushPcmI16(Int16Array)` — accepts a TypedArray view directly,
  no copy in either direction.
- `Detection.frameIndex` is a `bigint` (PCM frame counts can exceed
  2³² over long sessions); `.timestampSec` and `.score` are `number`.

## Where to go next

- C / C++ equivalent: `../../c/alsa-mic-quickstart/` (lives in the
  same v0.1.x release).
- Python quickstart: `../../python/quickstart/quickstart.py`.
