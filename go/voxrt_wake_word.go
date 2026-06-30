// Package wakeword wraps the VoxRT wake-word SDK for Linux aarch64.
//
// Import path: `github.com/VoxRT/voxrt-wake-word-linux/go`.
//
// Usage:
//
//	engine, err := wakeword.OpenFromPath("voxrt_wake_word.vxrt")
//	if err != nil { ... }
//	defer engine.Close()
//
//	engine.SetThreshold(0.9)
//	engine.SetCooldownFrames(100)
//
//	for chunk := range micCh {
//	    for _, d := range engine.PushPcmI16(chunk) {
//	        fmt.Printf("wake! t=%.3fs score=%.4f\n", d.TimestampSec, d.Score)
//	    }
//	}
//
// The native .so is bundled inside the module at
// lib/aarch64-linux-gnu/libvoxrt_wake_word.so. cgo's ${SRCDIR}
// variable resolves at build time so consumer binaries find it
// via a baked-in rpath — no LD_LIBRARY_PATH or system install
// needed.
package wakeword

/*
#cgo CFLAGS: -I${SRCDIR}/include
#cgo LDFLAGS: -L${SRCDIR}/lib/aarch64-linux-gnu -lvoxrt_wake_word -Wl,-rpath,${SRCDIR}/lib/aarch64-linux-gnu

#include <stdlib.h>
#include "voxrt_wake_word.h"
*/
import "C"

import (
	"errors"
	"fmt"
	"os"
	"runtime"
	"unsafe"
)

// Detection is one wake-word detection event. Field names and units
// match the Python (Detection), Kotlin (WakeWordDetection), and
// Swift (WakeWordDetection) bindings on the other channels.
type Detection struct {
	// FrameIndex is the 0-based frame index since session start (or
	// last Reset). One frame = 10 ms at 16 kHz.
	FrameIndex uint64
	// TimestampSec is seconds since session start at the detection
	// frame's start.
	TimestampSec float32
	// Score is the sigmoid score in [0, 1].
	Score float32
}

// Engine is a streaming wake-word inference session. Construct via
// OpenFromPath or OpenFromBytes; call Close when done. Engine is NOT
// safe for concurrent use — callers serialise their own access.
type Engine struct {
	handle *C.voxrt_wake_word_t
}

// OpenFromPath constructs an Engine from a .vxrt v2 model on disk.
func OpenFromPath(path string) (*Engine, error) {
	bytes, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("voxrt_wake_word: %s: %w", path, err)
	}
	return OpenFromBytes(bytes)
}

// OpenFromBytes constructs an Engine from raw .vxrt bytes (e.g.
// downloaded over the network or embedded as a resource).
func OpenFromBytes(bytes []byte) (*Engine, error) {
	if len(bytes) == 0 {
		return nil, errors.New("voxrt_wake_word: empty model bytes")
	}
	var handle *C.voxrt_wake_word_t
	rc := C.voxrt_wake_word_create(
		(*C.uint8_t)(unsafe.Pointer(&bytes[0])),
		C.size_t(len(bytes)),
		&handle,
	)
	if rc != C.VOXRT_OK {
		return nil, fmt.Errorf("voxrt_wake_word_create: status=%d", int32(rc))
	}
	e := &Engine{handle: handle}
	// Safety net: if the caller forgets to Close, the GC reclaims
	// the native handle eventually. NOT a substitute for explicit
	// Close — destructor ordering is non-deterministic.
	runtime.SetFinalizer(e, func(e *Engine) { e.Close() })
	return e, nil
}

// Close releases the native session handle. Subsequent method calls
// on this Engine are a no-op. Idempotent.
func (e *Engine) Close() {
	if e.handle == nil {
		return
	}
	C.voxrt_wake_word_destroy(e.handle)
	e.handle = nil
	runtime.SetFinalizer(e, nil)
}

// Reset wipes accumulated state (FIFO buffers, pre-emph carry,
// rolling pool, cooldown, frame counter). Subsequent pushes behave
// as if from a fresh session.
func (e *Engine) Reset() error {
	rc := C.voxrt_wake_word_reset(e.handle)
	if rc != C.VOXRT_OK {
		return fmt.Errorf("voxrt_wake_word_reset: status=%d", int32(rc))
	}
	return nil
}

// SetThreshold sets the sigmoid-space detection threshold. Range
// [0, 1]. Default is 0.9 (deployment baseline from v6 sweep).
func (e *Engine) SetThreshold(t float32) error {
	if t < 0 || t > 1 {
		return fmt.Errorf("voxrt_wake_word: threshold out of range [0, 1]: %f", t)
	}
	rc := C.voxrt_wake_word_set_threshold(e.handle, C.float(t))
	if rc != C.VOXRT_OK {
		return fmt.Errorf("voxrt_wake_word_set_threshold: status=%d", int32(rc))
	}
	return nil
}

// SetCooldownFrames sets the post-detection cooldown in frames (1
// frame = 10 ms). Default 100 = 1.0 s.
func (e *Engine) SetCooldownFrames(n int) error {
	rc := C.voxrt_wake_word_set_cooldown_frames(e.handle, C.size_t(n))
	if rc != C.VOXRT_OK {
		return fmt.Errorf("voxrt_wake_word_set_cooldown_frames: status=%d", int32(rc))
	}
	return nil
}

// CurrentScore returns the latest sigmoid score (0..1). Returns 0.5
// before any frame has been emitted.
func (e *Engine) CurrentScore() float32 {
	var score C.float
	C.voxrt_wake_word_current_score(e.handle, &score)
	return float32(score)
}

const detBufCap = 8

// PushPcmI16 pushes a chunk of int16 PCM (mono, 16 kHz, native
// endian) into the engine and returns any detections emitted during
// this chunk.
func (e *Engine) PushPcmI16(pcm []int16) []Detection {
	if len(pcm) == 0 {
		return nil
	}
	var dets [detBufCap]C.voxrt_wake_word_detection_t
	var written C.size_t
	C.voxrt_wake_word_push_pcm_i16(
		e.handle,
		(*C.int16_t)(unsafe.Pointer(&pcm[0])),
		C.size_t(len(pcm)),
		&dets[0],
		C.size_t(detBufCap),
		&written,
	)
	return goDetections(&dets, int(written))
}

// PushPcmF32 pushes a chunk of f32 PCM in [-1, 1] (mono, 16 kHz).
func (e *Engine) PushPcmF32(pcm []float32) []Detection {
	if len(pcm) == 0 {
		return nil
	}
	var dets [detBufCap]C.voxrt_wake_word_detection_t
	var written C.size_t
	C.voxrt_wake_word_push_pcm_f32(
		e.handle,
		(*C.float)(unsafe.Pointer(&pcm[0])),
		C.size_t(len(pcm)),
		&dets[0],
		C.size_t(detBufCap),
		&written,
	)
	return goDetections(&dets, int(written))
}

// Version returns the SDK version string.
func Version() string {
	return C.GoString(C.voxrt_wake_word_version())
}

func goDetections(dets *[detBufCap]C.voxrt_wake_word_detection_t, n int) []Detection {
	if n > detBufCap {
		n = detBufCap
	}
	out := make([]Detection, 0, n)
	for i := 0; i < n; i++ {
		d := dets[i]
		out = append(out, Detection{
			FrameIndex:   uint64(d.frame_index),
			TimestampSec: float32(d.timestamp_sec),
			Score:        float32(d.score),
		})
	}
	return out
}
