# SleepCube Audio Controller

Standalone ESP-IDF firmware for the AtomS3 Lite and ATOMIC Speaker Base.

M2 turned the M1-confirmed output path into a standalone finite-session audio
service. M4 adds the version-1 semantic UART endpoint while preserving local
storage, decoding, buffering, gain ramps, authoritative state and independent
deadline enforcement on this controller. The Waveshare can now request bounded
PLAY, STOP, SET_VOLUME and status through the contract in
`../../docs/software/05_ICD.md`; the Control product UI consumes that endpoint
through its remote audio-service facade.

## Toolchain

- ESP-IDF v6.0, commit `662a3be354759d9487bf4b1a629fadb766cb1800`
- target: `esp32s3`
- configured flash size: 8 MB

Source the ESP-IDF environment before using `idf.py`:

```sh
source /home/jeppe/esp/v6.0/esp-idf/export.sh
```

## Content card

The firmware accepts exactly this R00 content contract:

| Property | Required value |
| --- | --- |
| Card filesystem | FAT16 or FAT32 |
| Card path | `/sleepcube/audio.wav` |
| Container | RIFF/WAVE |
| Encoding | Linear PCM, signed 16-bit little-endian |
| Channels | Mono |
| Sample rate | 44,100 Hz |

The card is never automatically formatted and normal playback does not modify
its files. Existing files outside `/sleepcube/audio.wav` are ignored.

Generate a deterministic, loop-safe 60-second validation asset on the laptop:

```sh
python3 tools/generate_m2_test_wav.py /path/to/card/sleepcube/audio.wav \
  --seconds 60
```

The parent directory is created if needed. Unmount/eject the card cleanly,
insert it into the unpowered Speaker Base, and then connect USB power.

## Clean normal build

The normal image enforces a 60-minute session including its five-second timeout
fade:

```sh
idf.py -B /tmp/sleepcube-m2-build \
  -D SDKCONFIG=/tmp/sleepcube-m2-sdkconfig \
  set-target esp32s3 build
```

## Separate endurance build

The endurance image retains the explicit M2 verification identity while using
the same finite 60-minute deadline as the R00 product. It does not create a
runtime path for bypassing the finite timeout:

```sh
idf.py -B /tmp/sleepcube-m2-endurance-build \
  -D SDKCONFIG=/tmp/sleepcube-m2-endurance-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.endurance' \
  set-target esp32s3 build
```

## Matched touch-debug build

For the separate ADR-0011 Control touch grid, build the Audio endpoint with its
optional controlled-profile and generated-tone capability:

```sh
idf.py -B /tmp/sleepcube-audio-touch-debug-build \
  -D SDKCONFIG=/tmp/sleepcube-audio-touch-debug-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.touch-debug.defaults' \
  set-target esp32s3 build
```

This development image advertises optional semantic profile, tone and I2S-slot
controls. Copy these exact mono PCM files to `/sleepcube/i2s-test/` on the FAT
card (mounted by firmware as `/sdcard/sleepcube/i2s-test/`):

| File | Required format |
| --- | --- |
| `ambient_32000_16bit_mono_12s.wav` | 32,000 Hz, signed 16-bit LE, mono |
| `ambient_44100_16bit_mono_12s.wav` | 44,100 Hz, signed 16-bit LE, mono |
| `ambient_44100_24bit_mono_12s.wav` | 44,100 Hz, signed 24-bit LE, mono |
| `ambient_48000_16bit_mono_12s.wav` | 48,000 Hz, signed 16-bit LE, mono |
| `ambient_48000_24bit_mono_12s.wav` | 48,000 Hz, signed 24-bit LE, mono |
| `ambient_96000_16bit_mono_12s.wav` | 96,000 Hz, signed 16-bit LE, mono |

The touch grid cycles Music, then these profiles in the controlled order
44.1/16, 44.1/24, 48/16, 48/24, 32/16 and 96/16, followed by a generated
44.1 kHz C5 and then Music again.
It independently toggles native data/slot widths (`16/16`, `24/24`) and
32-bit-padded slots (`16/32`, `24/32`). Samples remain full precision in a Q31
gain path and are duplicated to both Philips-I2S channels. Missing or
mismatched files reject that selection without replacing the prior applied
profile. Both classic PCM and WAVE_FORMAT_EXTENSIBLE integer-PCM headers are
accepted when all declared format fields match the selected profile.

The generated C5 raw tone and retained gain cap combine for a maximum -24 dBFS
output at 100% volume. Profile/packing changes during playback fade down/up over
one second each while preserving the existing absolute session deadline. The
selection is volatile and returns to canonical music/C5/native slots when the
Control debug mode exits; volume remains persistent. The normal profile compiles
out the generator/profile catalog and does not advertise the capability.

## Flash and monitor

Connect the AtomS3 Lite over USB-C. Flashing requires two manual button actions:

1. Hold reset for about two seconds until the internal green LED lights, then
   release it to enter download mode.
2. Run the flash command and wait for completion.
3. Press reset once after flashing to leave download mode and start the app.

```sh
idf.py -B /tmp/sleepcube-m2-build \
  -D SDKCONFIG=/tmp/sleepcube-m2-sdkconfig \
  -p /dev/ttyACM0 flash
```

The application USB serial interface may re-enumerate under a different device
name. Attach without resetting it:

```sh
idf.py -B /tmp/sleepcube-m2-build \
  -D SDKCONFIG=/tmp/sleepcube-m2-sdkconfig \
  -p /dev/ttyACM1 monitor --no-reset
```

Exit with `Ctrl-]`. Port names are examples; identify the actual port each time.
Do not combine `flash monitor`, because this board needs the manual post-flash
reset between those operations.

## M4 two-controller semantic validation

M3 uses only Waveshare H1 pin 5 GPIO16 TX → Atom Grove yellow GPIO2 RX,
Atom Grove white GPIO1 TX → Waveshare H1 pin 7 GPIO17 RX, and H1 pin 6 GND →
Atom Grove black GND. Leave the Grove red 5 V contact disconnected. Power both
boards by their separate USB-C cables. Check header orientation before power.
The active framing/pin contract is in `../../docs/software/05_ICD.md`.

Build the Atom image with the normal command above. For the Waveshare, use its
separate M4 defaults so the existing UI/light service remains present but its
legacy local audio backend is disabled:

```sh
cd ../control
idf.py -B /tmp/sleepcube-m4-control-build \
  -D SDKCONFIG=/tmp/sleepcube-m4-control-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.m4.defaults' \
  set-target esp32c6 build
```

Identify both USB serial ports by stable `/dev/serial/by-id/` symlinks before
flashing; `/dev/ttyACM*` numbers can change. At startup the Control client
performs `HELLO` followed by `GET_STATUS`; no PLAY is issued automatically. The
Control USB console accepts single-character development commands:

- `p`: finite 30-second PLAY;
- `s`: STOP;
- `1`, `2`, `3`, `4`: SET_VOLUME to 25, 50, 75 or 100 percent;
- `q`: GET_STATUS;
- `r`: resend the last state-changing command ID to verify duplicate rejection.

This console exists only for M4 protocol validation. The touch UI remains
decoupled until M5. Startup and either-controller reset must remain silent; a
Control reset shall only reconcile status and shall not restart or extend audio.

## Standalone controls

Startup remains silent. Successful initialization logs the card identity,
validated content properties, retained I2S format, session duration and saved
volume before reporting `READY` in `STOPPED` state.

- short button press: toggle PLAY/STOP;
- hold for at least one second and release: cycle persistent volume through
  25%, 50%, 75% and 100%;
- reset/power cycle: always return to silent `STOPPED`; playback is not restored.

Start and manual stop use one-second linear gain ramps. A 250 ms ramp is used
for volume changes. The timeout fade begins five seconds before the finite
deadline so output reaches stopped/silent at the deadline. For R00, output gain
is quadratic over the user percentage and capped at -12 dBFS at 100%. The 1 s
start/manual-stop transition, 250 ms volume transition and 5 s timeout fade are
the validated R00 behavior. Any later retuning requires repeating the relevant
listening and transition evidence.

Playback logs a heartbeat every 60 seconds with state, exact frame/byte counts,
content-loop count and remaining session time. Any storage/read/I2S failure
latches a silent `FAULT` state and requires reboot after correcting the cause.

## Hardware mapping

| Function | AtomS3 Lite GPIO |
| --- | --- |
| I2S BCLK | GPIO5 |
| I2S LRCLK/WS | GPIO39 |
| I2S DATA | GPIO38 |
| microSD MOSI | GPIO6 |
| microSD MISO | GPIO8 |
| microSD CLK | GPIO7 |
| microSD CS | Fixed low on Speaker Base |
| Local button | GPIO41, active-low |

The amplifier-facing format remains the M1-confirmed 44.1 kHz signed 16-bit
Philips-I2S stereo format with each decoded mono sample duplicated into both
channel slots.
