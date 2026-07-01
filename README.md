# voxrt-wake-word for Linux

Always-on wake-phrase detection on the **VoxRT** custom on-device inference runtime. ~48K-parameter depthwise-separable convnet, 16 kHz mono in, sigmoid-score out + threshold-crossing events. Detects the phrase **"Hey Assistant"**.

- Current version: `v0.1.5`
- Target platform: `aarch64` Linux (Raspberry Pi 3 / 4 / 5 / Zero 2, NVIDIA Jetson, AWS Graviton, Rock Pi / Orange Pi / other aarch64 SBCs)
- glibc baseline: 2.17 (manylinux2014_aarch64 — Pi OS Bullseye/Bookworm, Jetson L4T 32.x/35.x/36.x, Ubuntu 18.04+, Debian 10+, RHEL 7+, AWS Graviton)
- Languages shipped: **Python** (PyPI wheel), **Node.js** (npm), **Go** (`go get`), **C / C++** (tarball + CMake + pkg-config), **Rust** (git dependency)
- License: Apache-2.0 (wrapper sources) · proprietary (compiled runtime, redistribution allowed via this SDK)
- Wake-phrase weights: proprietary in-house (synthetic training data; no upstream license obligations)

---

## What is VoxRT?

VoxRT is a from-scratch inference runtime for on-device speech models. No ONNX Runtime, no PyTorch Mobile, no LiteRT — a custom Rust core sized and tuned for streaming voice workloads on phone-class and edge-Linux hardware.

`voxrt-wake-word` is the wake-word product on that runtime, alongside [`voxrt-wake-word-android`](https://github.com/VoxRT/voxrt-wake-word-android) (JitPack) and [`voxrt-wake-word-ios`](https://github.com/VoxRT/voxrt-wake-word-ios) (Swift Package Manager). Same runtime crate, same NEON kernel set, same `.vxrt` model file — pick the SDK that matches your target. Sister products [`voxrt-silero`](https://github.com/VoxRT/voxrt-silero-android) (VAD) and [`voxrt-asr-android`](https://github.com/VoxRT/voxrt-asr-android) (streaming ASR) share the same runtime.

Custom-phrase wake-word models (your own brand name, multi-phrase detection, language extension) are part of the commercial VoxRT SDK tier. Contact help@voxrt.com.

## Model quality

Test split: 5,240 positive utterances + 6,416 hard-negative utterances (isolated "Hey", isolated "Assistant", competitor wake-words like "Hey Siri", phonetic neighbours, arbitrary speech, non-speech audio). All speakers disjoint from train + val.

- **ROC AUC: 0.9966**
- **Average precision (PR AUC): 0.9899**

| Threshold | Precision | Recall | F1     | FPR    | False positives on test |
| --------- | --------- | ------ | ------ | ------ | ----------------------- |
| 0.5       | 0.864     | 0.995  | 0.925  | 12.8 % | 822 / 6,416             |
| 0.85      | 0.957     | 0.987  | 0.972  | 3.7 %  | 234 / 6,416             |
| **0.9 (default)** | **0.993** | **0.982** | **0.987** | **0.5 %** | **34 / 6,416** |
| 0.95      | 0.997     | 0.769  | 0.868  | 0.2 %  | 12 / 6,416              |

The SDK ships with `threshold = 0.9` as the default operating point. Lower it at runtime if your application can tolerate more false positives in exchange for higher recall.

## Performance

`aarch64` release builds, post-warmup, live microphone at 16 kHz mono. RTF = wall-time-per-audio-second (lower is better):

| Device | CPU | RTF | CPU budget |
|---|---|---|---|
| Raspberry Pi Zero 2 W | Cortex-A53 ×4 @ 1.0 GHz | **0.053** (measured, sustained ≥ 60 s) | 5.3 % of one core |
| Raspberry Pi 3 A+ / B / B+ | Cortex-A53 ×4 @ 1.2–1.4 GHz | ~0.038–0.044 (scaled) | ~4 % |
| Raspberry Pi 4 B / 400 | Cortex-A72 ×4 @ 1.5–1.8 GHz | ~0.018–0.024 (scaled) | ~2 % |
| Raspberry Pi 5 | Cortex-A76 ×4 @ 2.4 GHz | ~0.008–0.012 (scaled) | ~1 % |
| NVIDIA Jetson Nano / Orin Nano | A57 / A78AE | ≲ Pi 4–5 | ≲ 2 % |
| AWS Graviton / Ampere Altra | Neoverse N1 / V1 | ≪ 0.005 | server-class |
| Rock Pi / Orange Pi / Khadas (A53/A55) | various | ≲ Pi 3–4 | ≲ 4 % |

The Pi Zero 2 W row is a direct measurement (RTF 0.053 sustained across a 60-second continuous live-mic session, peak-chunk RTF 0.074). Other rows scale by clock + µarch and are conservative — same binary, no per-device tuning.

At RTF ≈ 0.05 on the smallest Pi, the wake-word is **~19× faster than realtime** with 95 % of one core free for the rest of your app. Even on the LITTLE-cluster range, the ~5 % CPU budget survives thermal throttling gracefully.

## How it compares

The on-device wake-word category is dominated by Picovoice Porcupine on the paid side and openWakeWord on the OSS side:

| | **voxrt-wake-word** | Picovoice Porcupine | openWakeWord |
|---|---|---|---|
| Model file | **~100 KB** (`.vxrt`) | not published | not published |
| Edge-Linux RTF disclosed | ✅ **0.053 measured on Pi Zero 2 W** (5.3 % CPU); scaled figures for Pi 3/4/5 + Jetson + Graviton | ✅ **0.6 % CPU on Pi 5 32-bit** (published) | ❌ Pi 3 only, "15–20 models real-time" (no per-model RTF) |
| Accuracy headline | ROC AUC 0.9966 on "Hey Assistant"; precision 0.993 / recall 0.982 @ default threshold | 2.7 % miss rate averaged across 6 built-in keywords | varies per pretrained model |
| Native mobile SDK | ✅ Android JitPack + iOS SPM | ✅ Android + iOS + RN + Flutter | ❌ Python-only; community C++ port |
| License | Apache-2.0 wrapper + proprietary runtime + proprietary weights (redistribution allowed via this SDK) | Commercial (Free Plan evaluation-only; production tier opaque, sales-gated) | Apache-2.0 code, **CC-BY-NC-SA** on pretrained weights (non-commercial) |
| Custom phrase / language | Tuned per customer on request (paid engagement) | Via Picovoice Console — paid tier required for commercial deployment | Self-train via Colab + TTS (~1 hour) |

Differentiators: **published edge-Linux RTF on cheap hardware** (Porcupine only publishes RPi 5; we publish Pi Zero 2 W), a **~100 KB model file**, and **license clarity** (openWakeWord's NC weights block commercial use; Picovoice's paid tier is sales-gated).

Full sourced analysis at [voxrt.com](https://voxrt.com).

## Binary footprint

- **C tarball** (`voxrt-wake-word-0.1.5-aarch64-linux-gnu.tar.gz`): ~264 KB compressed. Contains `libvoxrt_wake_word.so` (~480 KB stripped), `voxrt_wake_word.h`, pkg-config + CMake configs, examples/, LICENSE × 2, README.
- **PyPI wheel** (`voxrt_wake_word-...-cp39-abi3-manylinux_2_17_aarch64.whl`): ~280 KB compressed. One wheel covers Python 3.9 / 3.10 / 3.11 / 3.12 / 3.13 via the pyo3 stable-ABI bridge.
- **npm package** (`voxrt-wake-word-0.1.5.tgz`, imports as `@voxrt/wake-word`): ~264 KB compressed. Contains `voxrt-wake-word.linux-arm64-gnu.node` (~480 KB) + `index.js` + `index.d.ts`.
- **Go module** (in-repo at `go/`): ~480 KB bundled `.so` + ~5 KB Go source. Fetched by `go get github.com/VoxRT/voxrt-wake-word-linux/go`.
- **Wake-phrase model** `voxrt_wake_word.vxrt` (downloaded separately from [voxrt-wake-word-models](https://github.com/VoxRT/voxrt-wake-word-models)): ~100 KB fp16.

Net effect on a shipped app: **roughly 600 KB** once the runtime `.so` + `.vxrt` + language wrapper are bundled.

## Install

### Python

```sh
pip install voxrt-wake-word
```

One `abi3` wheel covers Python 3.9 / 3.10 / 3.11 / 3.12 / 3.13. Or pin a specific version's wheel from a [GitHub Release](https://github.com/VoxRT/voxrt-wake-word-linux/releases/tag/v0.1.5) if your deploy pipeline prefers offline installs.

### Node.js

```sh
npm install @voxrt/wake-word
```

TypeScript definitions ship with the package. Or pin a specific version's tarball from a [GitHub Release](https://github.com/VoxRT/voxrt-wake-word-linux/releases/tag/v0.1.5) — same `.tgz`, offline-installable via `npm install ./…`.

### Go

```sh
go get github.com/VoxRT/voxrt-wake-word-linux/go@v0.1.5
```

The `go/` subdirectory of this repository IS the Go module — no separate publish step. cgo picks up the bundled `.so` via `${SRCDIR}`-relative rpath at build time; no `LD_LIBRARY_PATH` or system install required.

### C / C++

```sh
curl -L https://github.com/VoxRT/voxrt-wake-word-linux/releases/download/v0.1.5/voxrt-wake-word-0.1.5-aarch64-linux-gnu.tar.gz \
    | sudo tar -xz -C /usr/local --strip-components=1
sudo ldconfig
```

After extract:

```sh
gcc main.c $(pkg-config --cflags --libs voxrt-wake-word) -o my_app
```

Or in CMake:

```cmake
find_package(VoxRTWakeWord 0.1.5 REQUIRED)
target_link_libraries(my_app PRIVATE VoxRT::WakeWord)
```

### Rust

Until the crates.io decision is finalised (see [`docs/wake-word/12-linux-sdk-packaging.md`](https://github.com/make1986/ai-sdk/) in the source monorepo — private), pin the crate via git:

```toml
[dependencies]
voxrt-wake-word = { git = "https://github.com/VoxRT/voxrt-wake-word-linux", tag = "v0.1.5" }
```

## Get the wake-phrase model

The `voxrt_wake_word.vxrt` model file (AES-256-GCM encrypted, ~100 KB) is distributed separately from [voxrt-wake-word-models](https://github.com/VoxRT/voxrt-wake-word-models) so the SDK ships without weights and the two version independently. All `v0.1.x` SDK releases use the same **model `v0.1.0`**.

- **Download once at deploy time**:
  ```sh
  curl -LO https://github.com/VoxRT/voxrt-wake-word-models/releases/download/v0.1.0/voxrt_wake_word.vxrt
  ```
  Bundle alongside your application binary.

- **Download on first run + SHA-256 verify** — recommended if you want to swap models without a redeploy. Compare against `9d40bdc132a2ad8e85bd8a28bb49b77c51a7c62f60567222a037e44418510e8f`.

- **Embed as a build-time resource** — for Python: `importlib.resources`. For Node.js: `require.resolve` + `fs.readFileSync`. For Go: `//go:embed`. For C: any resource-embedder. Feed the bytes to `from_bytes(...)` on your language's `WakeWordEngine` constructor.

## Quick start

Python — the smallest possible end-to-end pipeline:

```python
from voxrt_wake_word import WakeWordEngine

engine = WakeWordEngine.from_path("voxrt_wake_word.vxrt")
engine.threshold = 0.9
engine.cooldown_frames = 100

for chunk in mic_iter():                # int16 mono @ 16 kHz, any chunk size
    for d in engine.push_pcm_i16(chunk):
        print(f"wake! t={d.timestamp_sec:.3f}s score={d.score:.4f}")
```

Every language ships an equivalent end-to-end example under [`examples/`](examples/). All quickstarts use only stdlib WAV parsing — no numpy / sounddevice / go-audio dependency.

## Examples

| Language | Example | What it does |
|---|---|---|
| C | [examples/c/alsa-mic-quickstart](examples/c/alsa-mic-quickstart) | Live ALSA microphone → wake-word events. Build with `make`, run against `plughw:0`. |
| C++ | [examples/c/cmake-consumer](examples/c/cmake-consumer) | Skeleton CMake project consuming the SDK via `find_package(VoxRTWakeWord ...)`. Copy-paste into a real project. |
| Python | [examples/python/quickstart](examples/python/quickstart) | Walk a 16 kHz WAV file in 32 ms chunks, print detections + RTF. Pure stdlib (`wave` + `struct`). |
| Node.js | [examples/nodejs/quickstart](examples/nodejs/quickstart) | Same shape, JavaScript. Manual WAV header parse via `fs` + `Buffer`, no npm deps. |
| Go | [examples/go/quickstart](examples/go/quickstart) | Same shape, Go. `encoding/binary` for the WAV header. cgo picks up the bundled `.so` via `${SRCDIR}` rpath. |

The C examples additionally ship with a live-microphone version; language quickstarts document the microphone-streaming pattern in each README (via `sounddevice` for Python, `naudiodon` / `portaudio` for Node, `portaudio` / `goalsa` for Go).

## Tuning

**Threshold.** Sigmoid-space `[0, 1]`. Default `0.9`. Lower increases recall (more detections, more false positives); raise increases precision (fewer detections, fewer false positives). See the Model quality table above for concrete precision/recall/FPR trade-offs at 0.5, 0.85, 0.9, 0.95.

**Cooldown.** Frames of suppression after a trigger. Default `100` (= 1.0 s at 10 ms hop). Prevents stuttering on a long wake phrase that crosses threshold across many consecutive frames. Lower for very tight utterance-to-utterance testing; leave at 100 for production.

**CPU affinity.** Not exposed as a public knob — the SDK does not pin threads on Linux. If you're on a big.LITTLE Pi (Pi 4/5) and want the wake-word thread to stick to the perf cluster, wrap `push_pcm_i16` in a thread you affine yourself via `sched_setaffinity` (POSIX C), `os.sched_setaffinity` (Python), or `runtime.LockOSThread` + `unix.SchedSetaffinity` (Go).

Both `threshold` and `cooldown_frames` are mutable at any time — change them between chunks safely.

## License

- Wrapper sources (Python / Node / Go / C headers / examples / docs): [Apache-2.0](LICENSE).
- Compiled binaries (`.so`, `.node`, `.whl`, prebuilt tarball artefact): proprietary, see [LICENSE-BINARY](LICENSE-BINARY).
- Wake-phrase model weights (`voxrt_wake_word.vxrt`): proprietary in-house (synthetic training data), same terms as LICENSE-BINARY.

Commercial licensing for custom phrases, additional languages, or bulk-device deployments: help@voxrt.com.

## Links

- Brand + docs: <https://voxrt.com>
- Android (JitPack): <https://github.com/VoxRT/voxrt-wake-word-android>
- iOS (Swift Package Manager): <https://github.com/VoxRT/voxrt-wake-word-ios>
- Wake-word model releases: <https://github.com/VoxRT/voxrt-wake-word-models>
- Issues: <https://github.com/VoxRT/voxrt-wake-word-linux/issues>
