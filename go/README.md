# voxrt-wake-word — Go binding

Go module: <https://pkg.go.dev/github.com/VoxRT/voxrt-wake-word-linux/go>

```sh
go get github.com/VoxRT/voxrt-wake-word-linux/go@v0.1.5
```

```go
package main

import (
    "fmt"

    wakeword "github.com/VoxRT/voxrt-wake-word-linux/go"
)

func main() {
    engine, err := wakeword.OpenFromPath("voxrt_wake_word.vxrt")
    if err != nil {
        panic(err)
    }
    defer engine.Close()
    engine.SetThreshold(0.9)

    for chunk := range micCh {
        for _, d := range engine.PushPcmI16(chunk) {
            fmt.Printf("wake! t=%.3fs score=%.4f\n", d.TimestampSec, d.Score)
        }
    }
}
```

Implemented via `cgo` over the same `libvoxrt_wake_word.so` shipped
in the C tarball. The shared object is bundled inside the module under
`go/lib/aarch64-linux-gnu/libvoxrt_wake_word.so` and the cgo linker
flags point at it automatically — no `apt install` step required on
the consumer machine.

**Toolchain:** Go ≥ 1.21 with cgo enabled (the default).
**Platform:** `aarch64-unknown-linux-gnu` only in v0.1 (see umbrella
README for the hardware coverage matrix).

**Version:** `v0.1.5`
**Model SHA-256:** `<see-releases-page>` (download separately from
[voxrt-wake-word-models](https://github.com/VoxRT/voxrt-wake-word-models)).

## Examples

(Linux/3.2 — TODO: ALSA mic example, channels-based stream wrapper,
gin-gonic / echo HTTP demo.)
