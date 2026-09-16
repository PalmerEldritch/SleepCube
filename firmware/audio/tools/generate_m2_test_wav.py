#!/usr/bin/env python3
"""Generate deterministic, loop-safe PCM WAV content for M2 validation."""

import argparse
import math
import struct
import wave
from pathlib import Path

SAMPLE_RATE = 44_100
AMPLITUDE = 16_384
BLOCK_FRAMES = 4_410


def write_test_wav(output: Path, duration_seconds: int) -> None:
    total_frames = SAMPLE_RATE * duration_seconds
    output.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(output), "wb") as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(SAMPLE_RATE)

        frame = 0
        while frame < total_frames:
            count = min(BLOCK_FRAMES, total_frames - frame)
            block = bytearray(count * 2)
            for index in range(count):
                position = frame + index
                # Integer-Hz components make every integer-second file loop at
                # a zero crossing without a discontinuity.
                sample = int(
                    AMPLITUDE
                    * (
                        0.65 * math.sin(2.0 * math.pi * 220.0 * position / SAMPLE_RATE)
                        + 0.35 * math.sin(2.0 * math.pi * 330.0 * position / SAMPLE_RATE)
                    )
                )
                struct.pack_into("<h", block, index * 2, sample)
            wav_file.writeframesraw(block)
            frame += count


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--seconds", type=int, default=60)
    args = parser.parse_args()
    if args.seconds <= 0:
        parser.error("--seconds must be positive")
    write_test_wav(args.output, args.seconds)


if __name__ == "__main__":
    main()
