// voxrt-wake-word — Node.js quickstart over a 16 kHz WAV file.
//
// Loads a .vxrt, walks a WAV file in 32 ms chunks, prints every wake
// detection + final RTF. Pure stdlib (no npm deps beyond
// @voxrt/wake-word itself) — uses Buffer + manual WAV header parse.
//
// Usage:
//   node quickstart.js voxrt_wake_word.vxrt hey_assistant.wav
//
// WAV must be 16 kHz mono 16-bit. Convert with:
//   ffmpeg -i input.flac -ar 16000 -ac 1 -sample_fmt s16 output.wav

"use strict";

const fs = require("fs");
const path = require("path");
const { WakeWordEngine } = require("@voxrt/wake-word");

const CHUNK_FRAMES = 512;  // 32 ms at 16 kHz

function readWavMono16(filePath) {
  const buf = fs.readFileSync(filePath);
  if (buf.toString("ascii", 0, 4) !== "RIFF" ||
      buf.toString("ascii", 8, 12) !== "WAVE") {
    throw new Error(`${filePath}: not a RIFF/WAVE file`);
  }
  // Walk chunks to find `fmt ` and `data`.
  let pos = 12;
  let sampleRate = 0;
  let channels = 0;
  let bitsPerSample = 0;
  let dataOffset = 0;
  let dataSize = 0;
  while (pos < buf.length) {
    const id = buf.toString("ascii", pos, pos + 4);
    const size = buf.readUInt32LE(pos + 4);
    if (id === "fmt ") {
      channels = buf.readUInt16LE(pos + 10);
      sampleRate = buf.readUInt32LE(pos + 12);
      bitsPerSample = buf.readUInt16LE(pos + 22);
    } else if (id === "data") {
      dataOffset = pos + 8;
      dataSize = size;
      break;
    }
    pos += 8 + size;
  }
  if (sampleRate !== 16000 || channels !== 1 || bitsPerSample !== 16) {
    throw new Error(
      `${filePath}: must be 16 kHz mono 16-bit, got ${sampleRate} Hz / ` +
      `${channels} channel(s) / ${bitsPerSample}-bit`
    );
  }
  return new Int16Array(
    buf.buffer,
    buf.byteOffset + dataOffset,
    dataSize / 2,
  );
}

function main(modelPath, wavPath) {
  console.log("VoxRT wake-word — Node.js quickstart");
  console.log(`  model: ${modelPath}`);
  console.log(`  audio: ${wavPath}\n`);

  const engine = WakeWordEngine.fromPath(modelPath);
  engine.threshold = 0.9;
  engine.cooldownFrames = 100;

  const samples = readWavMono16(wavPath);
  console.log(`  duration: ${(samples.length / 16000).toFixed(2)} s\n`);

  const start = process.hrtime.bigint();
  let wakes = 0;
  for (let off = 0; off < samples.length; off += CHUNK_FRAMES) {
    const chunk = samples.subarray(off, off + CHUNK_FRAMES);
    for (const d of engine.pushPcmI16(chunk)) {
      wakes++;
      console.log(
        `  [wake] frame=${d.frameIndex} ` +
        `t=${d.timestampSec.toFixed(3)}s ` +
        `score=${d.score.toFixed(4)}`
      );
    }
  }
  const elapsedNs = Number(process.hrtime.bigint() - start);
  const elapsed = elapsedNs / 1e9;
  const audioSec = samples.length / 16000;

  console.log();
  console.log(
    `  processed ${audioSec.toFixed(2)} s of audio in ${elapsed.toFixed(3)} s wall-clock`
  );
  console.log(
    `  RTF = ${(elapsed / audioSec).toFixed(4)} (${wakes} wake event(s) emitted)`
  );
}

if (process.argv.length !== 4) {
  console.error("usage: node quickstart.js <model.vxrt> <audio.wav>");
  process.exit(1);
}
main(process.argv[2], process.argv[3]);
