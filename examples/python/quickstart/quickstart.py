"""voxrt-wake-word quickstart — load a .wav, run wake-word over it.

Demonstrates the smallest possible end-to-end pipeline:

  1. Construct WakeWordEngine from a .vxrt path.
  2. Read a 16 kHz mono WAV file in 512-sample chunks (32 ms each).
  3. Push each chunk through push_pcm_i16, print any detections.

Pure stdlib — uses `wave` to read the WAV so the example works on a
fresh Raspberry Pi without `numpy` / `soundfile` / `sounddevice`.

Usage:
    python3 quickstart.py voxrt_wake_word.vxrt my_recording.wav

The WAV must be 16 kHz, mono, 16-bit PCM. Convert with:
    ffmpeg -i input.flac -ar 16000 -ac 1 -sample_fmt s16 output.wav

For live-microphone streaming, install `sounddevice` (`pip install
sounddevice`) and feed `rec(samplerate=16000, channels=1, dtype='int16')`
chunks into `engine.push_pcm_i16` — same API.
"""

import struct
import sys
import wave
from time import perf_counter

from voxrt_wake_word import WakeWordEngine

CHUNK_FRAMES = 512  # 32 ms at 16 kHz


def main(model_path: str, wav_path: str) -> None:
    print(f"VoxRT wake-word — quickstart")
    print(f"  model: {model_path}")
    print(f"  audio: {wav_path}")
    print()

    engine = WakeWordEngine.from_path(model_path)
    engine.threshold = 0.9
    engine.cooldown_frames = 100

    with wave.open(wav_path, "rb") as wav:
        sr = wav.getframerate()
        channels = wav.getnchannels()
        sampwidth = wav.getsampwidth()
        if sr != 16000 or channels != 1 or sampwidth != 2:
            sys.exit(
                f"quickstart: WAV must be 16 kHz mono 16-bit — "
                f"got rate={sr} channels={channels} sampwidth={sampwidth}"
            )
        total_frames = wav.getnframes()
        print(f"  duration: {total_frames / sr:.2f} s")
        print()

        start = perf_counter()
        wakes = 0
        remaining = total_frames
        while remaining > 0:
            n = min(CHUNK_FRAMES, remaining)
            raw = wav.readframes(n)
            samples = struct.unpack(f"<{n}h", raw)
            for d in engine.push_pcm_i16(list(samples)):
                wakes += 1
                print(f"  [wake] frame={d.frame_index} "
                      f"t={d.timestamp_sec:.3f}s score={d.score:.4f}")
            remaining -= n
        elapsed = perf_counter() - start

    audio_sec = total_frames / 16000.0
    print()
    print(f"  processed {audio_sec:.2f} s of audio in {elapsed:.3f} s wall-clock")
    print(f"  RTF = {elapsed / audio_sec:.4f} ({wakes} wake event(s) emitted)")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: quickstart.py <model.vxrt> <audio.wav>")
    main(sys.argv[1], sys.argv[2])
