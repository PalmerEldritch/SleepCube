# M3 UART transport bring-up — development record

**Date:** 2026-09-21
**Status:** Software/host gate passes; both images flashed and bidirectional
PING/PONG verified; M3 fault/reset/link-loss gate pending.

## Scope and configuration

M3 is transport-only. It does not implement or issue remote PLAY, STOP, volume,
status or session commands. The Atom keeps its M2 local button/audio behavior.
The Waveshare M3 build keeps UI and lighting enabled but disables its legacy
local audio backend. The exact crossed 3.3 V UART mapping and no-5-V rule are
specified in `../05_ICD.md`; framing and diagnostic type IDs are in ADR-0007.

Both modules were reported connected to separate laptop USB-C power cables.
USB enumeration on this date showed two Espressif USB Serial/JTAG devices:

| Stable USB serial ID suffix | Current path | Identification |
| --- | --- | --- |
| `44:1B:F6:F4:FE:C8` | `/dev/ttyACM1` | AtomS3 Lite, matched to prior M1 target record |
| `AC:EB:E6:1E:67:C4` | `/dev/ttyACM0` | Waveshare, confirmed by ESP32-C6FH8 chip identification during flash |

The user confirmed the jumper connections after the build preparation. Their
orientation, absence of a 5 V bridge, and electrical operation have **not**
been independently inspected or verified. USB enumeration does not prove UART
wiring.

## Software evidence

The shared protocol codec is host-tested with `cc -std=c11 -Wall -Wextra
-Werror -fsanitize=undefined`. Round trip at maximum payload, malformed COBS,
invalid version/length/CRC, incomplete encoding, overlong packet discard,
inter-byte timeout, and delimiter resynchronization pass. AddressSanitizer
could not run under the restricted tool environment because LeakSanitizer
requires unsupported process tracing; the uninstrumented/UBSan test passes.

ESP-IDF v6.0 at commit `662a3be354759d9487bf4b1a629fadb766cb1800`
builds both M3 images from tracked source and defaults using external build and
configuration paths:

```sh
cd firmware/audio
idf.py -B /tmp/sleepcube-m3-audio-build \
  -D SDKCONFIG=/tmp/sleepcube-m3-audio-sdkconfig set-target esp32s3 build

cd ../control
idf.py -B /tmp/sleepcube-m3-control-safe-build \
  -D SDKCONFIG=/tmp/sleepcube-m3-control-safe-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.m3.defaults' \
  set-target esp32c6 build
```

The Atom build targets ESP32-S3/8 MB and the Waveshare test build targets
ESP32-C6/8 MB, USB Serial/JTAG console, UI/light enabled, old local audio
disabled. Both initial clean builds and subsequent incremental rebuilds pass;
neither reported a project-code compiler warning. After the final transport
task change, both incremental rebuilds also pass. The current application
images are 338,656 bytes (Audio) and 595,200 bytes (Control). Record the source
commit, final image sizes and hashes again at the flash session: commit-version
metadata can change the images after this software-preparation build.

## Pending hardware gate

1. Confirm crossed TX/RX, common ground, disconnected/insulated Grove red 5 V,
   card/speaker connection, and separate USB power. Avoid hot-rewiring.
2. Identify both boards by stable USB ID; enter Atom download mode manually by
   holding reset about two seconds. Flash its M3 image, then press reset again
   to start the application. Flash the Waveshare M3 UI/light-safe image using
   its identified port and board-specific procedure.
3. Capture complete boot/application logs from both USB consoles. Confirm
   silent Atom startup, normal card/content validation, UI/lighting still
   responsive, and repeated matched PING/PONG in both directions.
4. After three matches per side the harness sends one deliberately bad-CRC and
   one time-expired partial probe in each direction. Confirm receiver rejection,
   no playback-state change/panic/watchdog/reboot, and later valid matches.
5. Independently reset each controller and test both boot orders and a brief
   UART disconnection/reconnection without changing the Atom's finite session
   behavior. Record exact counts and logs before declaring M3 complete.

## Atom flash — 2026-09-21

After the user confirmed the jumper hookup and put the Atom in download mode,
the M3 Audio image from source commit `5894e14` was flashed through stable USB
ID `44:1B:F6:F4:FE:C8` (then `/dev/ttyACM1`). `esptool` identified ESP32-S3
revision v0.2 with embedded 8 MB flash. Bootloader, partition table and the
338,656-byte application image all completed hash verification. The exact
post-commit application SHA-256 before flash was
`6b20cb050b048461687745a7e93fee26e24629511731f91f1bb72bc3d5af9bf3`.
The tool reported `Hard resetting via RTS pin`. The user then pressed Atom
reset and reported that the application started. A no-reset USB monitor attached
after startup captured the M3 audio transport repeatedly sending PINGs
(sequences 32778–32782) and reporting missing PONGs. This is expected while
the Waveshare still runs its earlier firmware; it is not link-pass evidence.
The monitor caught only a partial application identity line, so a complete
post-reset boot log, silent-start confirmation and card/content validation are
still pending recapture.

At this stage, before the Waveshare flash, no on-target UART, malformed-frame,
reset, link-loss, UI or listening result was claimed. Later evidence follows.
`AUD-ISS-001` is separate and remains open.

## Waveshare download-mode attempt — 2026-09-21

The user reported the Waveshare ready for flashing after the documented
BOOT/RESET sequence. Immediately before the write, stable USB ID
`AC:EB:E6:1E:67:C4` and `/dev/ttyACM0` were absent; only the Atom remained.
`lsusb` showed one Espressif USB Serial/JTAG device. Kernel events showed the
Waveshare disconnecting at USB path `3-4.1`, followed by an unsuccessful
reenumeration (`device not accepting address`, error `-71`, and `unable to
enumerate USB device`). **No Waveshare flash command was run and no Waveshare
flash content was changed.** This is a USB enumeration interruption, not
evidence of an M3 UART or firmware failure. Next safe action: ask the user to
press Waveshare RESET once without unplugging either powered board, then
reinspect the stable USB identity. Avoid unplugging one powered controller while
its UART jumpers remain attached to the other.

## Waveshare flash and first bidirectional link — 2026-09-21

The user pressed Waveshare RESET without disconnecting cables. Stable USB ID
`AC:EB:E6:1E:67:C4` returned as `/dev/ttyACM0`. The previously authorized M3
Control flash then succeeded through that stable path using ESP-IDF's automatic
reset. `esptool` identified ESP32-C6FH8 revision v0.2 with embedded 8 MB
flash. Bootloader, partition table and 595,200-byte application writes all
passed hash verification. The image was built from source commit `86d8e3e`;
its SHA-256 was
`5efe50d8adb9790242e37985d68eaf2850d273f0c877ac9cecc9ae90b680afcd`.
The flash command did not write the Control storage partition.

No-reset USB monitors then captured repeated valid traffic in both directions:
the Atom matched PONGs through `total=21` (including `seq=32956`) and echoed
Control PINGs; the Waveshare matched through `total=21` (including `seq=22`)
and echoed Atom PINGs. Matching responses arrived approximately 30 ms after
their logged PINGs in representative samples. No timeout, parser rejection,
panic or reboot appeared in these captured intervals. This directly verifies
the crossed UART path and basic bidirectional framing. The monitors attached
after each board's one-time malformed/truncated probes, so their injection and
post-error recovery are still **unverified on target**. Full boot logs, visual
UI/lighting continuity, independent reset and link-loss tests also remain
pending. M3 has not passed its complete exit gate.
