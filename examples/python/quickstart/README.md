# quickstart — Python wake-word demo over a 16 kHz WAV file

Smallest possible Python pipeline: load the `.vxrt`, walk a WAV file
in 32 ms chunks, print every wake-word detection. Pure stdlib — no
`numpy` / `soundfile` / `sounddevice` needed.

## Install

```sh
python3 -m pip install voxrt-wake-word
```

(Not yet on PyPI as of v0.1.2 — for now download the wheel from
[this release](https://github.com/VoxRT/voxrt-wake-word-linux/releases/tag/v0.1.2):)

```sh
curl -LO https://github.com/VoxRT/voxrt-wake-word-linux/releases/download/v0.1.2/voxrt_wake_word-0.1.2-cp39-abi3-manylinux_2_17_aarch64.manylinux2014_aarch64.whl
python3 -m pip install ./voxrt_wake_word-0.1.2-cp39-abi3-manylinux_2_17_aarch64.manylinux2014_aarch64.whl
```

Get the wake-phrase model:

```sh
curl -LO https://github.com/VoxRT/voxrt-wake-word-models/releases/download/v0.1.0/voxrt_wake_word.vxrt
```

## Run

```sh
# Convert any audio to 16 kHz mono 16-bit first if needed:
ffmpeg -i hey_assistant.flac -ar 16000 -ac 1 -sample_fmt s16 hey_assistant.wav

python3 quickstart.py voxrt_wake_word.vxrt hey_assistant.wav
```

Expected output:

```
VoxRT wake-word — quickstart
  model: voxrt_wake_word.vxrt
  audio: hey_assistant.wav

  duration: 1.50 s

  [wake] frame=132 t=1.320s score=0.9907

  processed 1.50 s of audio in 0.080 s wall-clock
  RTF = 0.0533 (1 wake event(s) emitted)
```

## Live microphone

For real-time streaming, install `sounddevice`:

```sh
python3 -m pip install sounddevice
```

then in your code:

```python
import sounddevice as sd
from voxrt_wake_word import WakeWordEngine

engine = WakeWordEngine.from_path("voxrt_wake_word.vxrt")

def callback(indata, frames, time_info, status):
    for d in engine.push_pcm_i16(indata[:, 0].tolist()):
        print(f"wake! t={d.timestamp_sec:.3f}s score={d.score:.4f}")

with sd.InputStream(samplerate=16000, channels=1, dtype="int16",
                    blocksize=512, callback=callback):
    sd.sleep(60_000)
```

## What this teaches

- `WakeWordEngine.from_path` / `WakeWordEngine.from_bytes` — two
  constructors mirroring the C ABI.
- `engine.threshold = 0.9` / `engine.cooldown_frames = 100` — Python
  properties, mutable at any time.
- `engine.push_pcm_i16(list[int])` — accepts any Python sequence of
  int16 values. For numpy arrays use `arr.tolist()` or `arr.astype(int).tolist()`.
- `Detection.frame_index` / `.timestamp_sec` / `.score` — same shape
  as Android (Kotlin) and iOS (Swift) `WakeWordDetection`.

## Where to go next

- For the C / C++ equivalent that streams from an ALSA microphone,
  see `../../c/alsa-mic-quickstart/` in the same release tarball.
