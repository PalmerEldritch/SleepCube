# M3 UART transport bring-up — development record

**Date:** 2026-09-21
**Status:** Software/host gate passes; two-controller hardware gate pending.

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
| `AC:EB:E6:1E:67:C4` | `/dev/ttyACM0` | Waveshare, by exclusion; verify before flashing |

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

No on-target UART, malformed-frame, reset, link-loss, UI or listening result is
claimed by this report yet. `AUD-ISS-001` is separate and remains open.
