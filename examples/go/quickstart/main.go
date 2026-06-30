// voxrt-wake-word — Go quickstart over a 16 kHz WAV file.
//
// Loads a .vxrt, walks a WAV file in 32 ms chunks, prints every wake
// detection + final RTF. Standard-library only — uses the `encoding/
// binary` package to parse the WAV header by hand.
//
// Build:
//   go build .
//   ./quickstart voxrt_wake_word.vxrt hey_assistant.wav
//
// WAV must be 16 kHz mono 16-bit. Convert with:
//   ffmpeg -i input.flac -ar 16000 -ac 1 -sample_fmt s16 output.wav

package main

import (
	"encoding/binary"
	"fmt"
	"os"
	"time"

	wakeword "github.com/VoxRT/voxrt-wake-word-linux/go"
)

const chunkFrames = 512 // 32 ms at 16 kHz

func main() {
	if len(os.Args) != 3 {
		fmt.Fprintln(os.Stderr, "usage: quickstart <model.vxrt> <audio.wav>")
		os.Exit(1)
	}
	modelPath, wavPath := os.Args[1], os.Args[2]

	fmt.Println("VoxRT wake-word — Go quickstart")
	fmt.Printf("  model: %s\n", modelPath)
	fmt.Printf("  audio: %s\n\n", wavPath)

	engine, err := wakeword.OpenFromPath(modelPath)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(2)
	}
	defer engine.Close()
	_ = engine.SetThreshold(0.9)
	_ = engine.SetCooldownFrames(100)

	samples, err := readWavMono16(wavPath)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(3)
	}
	fmt.Printf("  duration: %.2f s\n\n", float64(len(samples))/16000)

	start := time.Now()
	wakes := 0
	for off := 0; off < len(samples); off += chunkFrames {
		end := off + chunkFrames
		if end > len(samples) {
			end = len(samples)
		}
		for _, d := range engine.PushPcmI16(samples[off:end]) {
			wakes++
			fmt.Printf("  [wake] frame=%d t=%.3fs score=%.4f\n",
				d.FrameIndex, d.TimestampSec, d.Score)
		}
	}
	elapsed := time.Since(start)
	audioSec := float64(len(samples)) / 16000
	fmt.Println()
	fmt.Printf("  processed %.2f s of audio in %.3f s wall-clock\n", audioSec, elapsed.Seconds())
	fmt.Printf("  RTF = %.4f (%d wake event(s) emitted)\n", elapsed.Seconds()/audioSec, wakes)
}

func readWavMono16(path string) ([]int16, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	header := make([]byte, 12)
	if _, err := f.Read(header); err != nil {
		return nil, fmt.Errorf("%s: %w", path, err)
	}
	if string(header[:4]) != "RIFF" || string(header[8:12]) != "WAVE" {
		return nil, fmt.Errorf("%s: not a RIFF/WAVE file", path)
	}

	var (
		sampleRate    uint32
		channels      uint16
		bitsPerSample uint16
		dataSize      uint32
	)
	for {
		chunkHdr := make([]byte, 8)
		n, err := f.Read(chunkHdr)
		if err != nil || n != 8 {
			return nil, fmt.Errorf("%s: malformed WAV (no data chunk)", path)
		}
		id := string(chunkHdr[:4])
		size := binary.LittleEndian.Uint32(chunkHdr[4:8])
		if id == "fmt " {
			fmtData := make([]byte, size)
			if _, err := f.Read(fmtData); err != nil {
				return nil, err
			}
			channels = binary.LittleEndian.Uint16(fmtData[2:4])
			sampleRate = binary.LittleEndian.Uint32(fmtData[4:8])
			bitsPerSample = binary.LittleEndian.Uint16(fmtData[14:16])
		} else if id == "data" {
			dataSize = size
			break
		} else {
			if _, err := f.Seek(int64(size), 1); err != nil {
				return nil, err
			}
		}
	}
	if sampleRate != 16000 || channels != 1 || bitsPerSample != 16 {
		return nil, fmt.Errorf(
			"%s: must be 16 kHz mono 16-bit, got %d Hz / %d channel(s) / %d-bit",
			path, sampleRate, channels, bitsPerSample,
		)
	}

	buf := make([]byte, dataSize)
	if _, err := f.Read(buf); err != nil {
		return nil, err
	}
	samples := make([]int16, dataSize/2)
	for i := range samples {
		samples[i] = int16(binary.LittleEndian.Uint16(buf[i*2 : i*2+2]))
	}
	return samples, nil
}
