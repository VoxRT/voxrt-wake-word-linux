# voxrt-wake-word — Linux SDK

On-device wake-word detection for **aarch64 Linux**, MIT-licensed runtime, AES-256-GCM encrypted weights. Ships under one runtime for **Python**, **Node.js**, **Go**, **C/C++**, and **Rust** — drop into any production stack on any 64-bit Linux device.

Companion to [`voxrt-wake-word-android`](https://github.com/VoxRT/voxrt-wake-word-android) and [`voxrt-wake-word-ios`](https://github.com/VoxRT/voxrt-wake-word-ios). Same `.vxrt` model file works on every platform.

**Version:** `v0.1.0`
**Model SHA-256:** `9d40bdc132a2ad8e85bd8a28bb49b77c51a7c62f60567222a037e44418510e8f`

## Hardware coverage

One `linux_aarch64` build, every modern aarch64 Linux device:

| Device | CPU | RTF | CPU budget | Status |
|---|---|---|---|---|
| Raspberry Pi Zero 2 W | A53 ×4 @ 1.0 GHz | **0.053** | 5.3 % | ✅ measured |
| Raspberry Pi 3 A+/B/B+ | A53 ×4 @ 1.2-1.4 GHz | ~0.038-0.044 | ~4 % | ✅ scaled |
| Raspberry Pi 4 B / 400 | A72 ×4 @ 1.5-1.8 GHz | ~0.018-0.024 | ~2 % | ✅ scaled |
| Raspberry Pi 5 | A76 ×4 @ 2.4 GHz | ~0.008-0.012 | ~1 % | ✅ scaled |
| NVIDIA Jetson Nano / Orin Nano | A57/A78AE | ≲ Pi 4-5 | ≲ 2 % | ✅ aarch64 + glibc |
| AWS Graviton / Ampere Altra | Neoverse N1/V1 | ≪ 0.005 | server-class | ✅ aarch64 + glibc |
| Rock Pi, Orange Pi, Khadas (A53/A55) | various | ≲ Pi 3-4 | ≲ 4 % | ✅ aarch64 + glibc |

Older 32-bit Pi (Pi 1, Zero, Zero W, Pi 2 rev 1.x) are **not supported** in v0.1 — add an issue if you need ARMv7 / ARMv6 coverage.

**glibc baseline: 2.17** (manylinux2014_aarch64). Covers Pi OS Bullseye and Bookworm, Jetson L4T 32.x and 35.x+, Ubuntu 18.04+, Debian 10+, RHEL/CentOS 7+, Amazon Linux 2 and 2023.

## Quickstart

### Python

```sh
pip install voxrt-wake-word
```

```python
from voxrt_wake_word import WakeWordEngine

engine = WakeWordEngine.from_path("voxrt_wake_word.vxrt")
engine.threshold = 0.9

for chunk in microphone_iter():           # numpy int16, 16 kHz mono
    for d in engine.push_pcm_i16(chunk):
        print(f"wake! t={d.timestamp_sec:.3f}s score={d.score:.4f}")
```

See [`bindings/python/README.md`](bindings/python/README.md).

### Node.js

```sh
npm install @voxrt/wake-word
```

```js
const { WakeWordEngine } = require("@voxrt/wake-word");

const engine = WakeWordEngine.fromPath("voxrt_wake_word.vxrt");
engine.threshold = 0.9;

for await (const chunk of micStream) {
  for (const d of engine.pushPcmI16(chunk)) {
    console.log(`wake! t=${d.timestampSec.toFixed(3)}s score=${d.score.toFixed(4)}`);
  }
}
```

See [`bindings/nodejs/README.md`](bindings/nodejs/README.md).

### Go

```sh
go get github.com/VoxRT/voxrt-wake-word-linux/go
```

```go
import wakeword "github.com/VoxRT/voxrt-wake-word-linux/go"

engine, _ := wakeword.OpenFromPath("voxrt_wake_word.vxrt")
defer engine.Close()
engine.SetThreshold(0.9)

for chunk := range micCh {
    for _, d := range engine.PushPcmI16(chunk) {
        fmt.Printf("wake! t=%.3fs score=%.4f\n", d.TimestampSec, d.Score)
    }
}
```

See [`bindings/go/README.md`](bindings/go/README.md).

### C / C++

Download `voxrt-wake-word-0.1.0-aarch64-linux-gnu.tar.gz` from the [latest GitHub Release](https://github.com/VoxRT/voxrt-wake-word-linux/releases/latest).

```c
#include <voxrt_wake_word.h>

voxrt_wake_word_session_t *engine =
    voxrt_wake_word_session_open("voxrt_wake_word.vxrt");
voxrt_wake_word_session_set_threshold(engine, 0.9f);

int16_t buf[512];
while (mic_read(buf, 512)) {
    voxrt_wake_word_detection_t out[8];
    size_t n = voxrt_wake_word_session_push_pcm_i16(
        engine, buf, 512, out, 8);
    for (size_t i = 0; i < n; ++i) {
        printf("wake! t=%.3fs score=%.4f\n",
               out[i].timestamp_sec, out[i].score);
    }
}
voxrt_wake_word_session_close(engine);
```

See [`bindings/c/README.md`](bindings/c/README.md).

### Rust

```sh
cargo add voxrt-wake-word
```

```rust
use voxrt_wake_word::{WakeWordSession, WakeWordSessionConfig};

let cfg = WakeWordSessionConfig::default();
let mut engine = WakeWordSession::open("voxrt_wake_word.vxrt")?;
engine.set_threshold(0.9);

for chunk in mic_iter {
    for d in engine.push_pcm_i16(&chunk) {
        println!("wake! t={:.3}s score={:.4}", d.timestamp_sec, d.score);
    }
}
```

## What's inside

- **Same Rust runtime** as the official VoxRT Android (JitPack) + iOS (SPM) SDK modules — the binary is built from the same `voxrt-wake-word` Rust crate.
- **MIT runtime source, encrypted weights** — runtime under `LICENSE-RUNTIME-MIT`, prebuilt `.so` under `LICENSE-BINARY`. The `.vxrt` model file is AES-256-GCM encrypted, master-key XOR-split at build time per ADR-0023.
- **Anti-RE hardening:** `obfstr` on internal strings, linker version script (only `voxrt_wake_word_*` symbols visible), ELF metadata stripped (no RUNPATH, no build-id, no `.comment` rustc fingerprint), anti-`ptrace` startup check, `.text`-section integrity hash. See `docs/anti-re.md`.
- **Custom wake phrase** = commercial SDK tier. Default phrase "Hey Assistant" ships free. Contact help@voxrt.com for custom-phrase model training.

## License

- **Wrapper sources** (Python wrapper, Node wrapper, Go wrapper, C examples, docs): Apache 2.0, see [LICENSE](LICENSE).
- **Compiled binaries** (`.so`, `.node`, `.vxrt`): proprietary, see [LICENSE-BINARY](LICENSE-BINARY).

## Links

- VoxRT brand + docs: <https://voxrt.com>
- Android (JitPack): <https://github.com/VoxRT/voxrt-wake-word-android>
- iOS (SPM): <https://github.com/VoxRT/voxrt-wake-word-ios>
- Models: <https://github.com/VoxRT/voxrt-wake-word-models>
- Issues: <https://github.com/VoxRT/voxrt-wake-word-linux/issues>
- Commercial: help@voxrt.com
