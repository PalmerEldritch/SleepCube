# SleepCube Control Controller

ESP-IDF firmware for the Waveshare ESP32-C6 touch/display controller. The
normal product build owns touch UI, display and GPIO8 ambient lighting, and
uses the semantic UART facade to control the separate Audio Controller.

## Toolchain and wiring

- ESP-IDF v6.0, commit `662a3be354759d9487bf4b1a629fadb766cb1800`
- target: `esp32c6`
- configured flash size: 8 MB
- Control GPIO16 TX → Audio GPIO2 RX
- Audio GPIO1 TX → Control GPIO17 RX
- common ground; leave the Audio Grove 5 V contact disconnected
- power both controllers independently through USB-C

Source ESP-IDF before building:

```sh
source /home/jeppe/esp/v6.0/esp-idf/export.sh
```

## Clean product build

The R00 product profile uses the M7-verified UI/lighting configuration and
requests a finite 1,800-second Audio session:

```sh
idf.py -B /tmp/sleepcube-control-product-build \
  -D SDKCONFIG=/tmp/sleepcube-control-product-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.m7.defaults' \
  set-target esp32c6 build
```

The timing profile adds structured touch/application traces while preserving
the product duration and behavior:

```sh
idf.py -B /tmp/sleepcube-control-trace-build \
  -D SDKCONFIG=/tmp/sleepcube-control-trace-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.m7.defaults;sdkconfig.m7.trace' \
  set-target esp32c6 build
```

The M8 endurance overlay changes only the finite requested duration to 3,600
seconds. It is a test image, not the release product:

```sh
idf.py -B /tmp/sleepcube-control-endurance-build \
  -D SDKCONFIG=/tmp/sleepcube-control-endurance-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.m7.defaults;sdkconfig.m8.endurance' \
  set-target esp32c6 build
```

## Flash and monitor

Identify the Waveshare by its stable entry under `/dev/serial/by-id/`; do not
assume `/dev/ttyACM0`. The ESP32-C6 normally enters download mode automatically:

```sh
idf.py -B /tmp/sleepcube-control-product-build \
  -D SDKCONFIG=/tmp/sleepcube-control-product-sdkconfig \
  -p /dev/ttyACM0 flash

idf.py -B /tmp/sleepcube-control-product-build \
  -D SDKCONFIG=/tmp/sleepcube-control-product-sdkconfig \
  -p /dev/ttyACM0 monitor --no-reset
```

Exit the monitor with `Ctrl-]`. Flashing the Control Controller does not require
flashing the Audio Controller when the ICD semantic version is unchanged. NVS
is preserved by a normal application flash.

## M8 timing capture

Flash the timing profile, capture a nominal run containing at least 20 touch
actions and 20 acknowledged audio commands, then analyze the saved monitor log:

```sh
python3 tools/analyze_m8_timing.py /path/to/control-monitor.log
```

The analyzer checks the maximum UI-event queue time against 100 ms and the
complete user-event-to-correlated-acknowledgement time against 250 ms. It also
reports the UART request/response RTT. Slider traffic is intentionally
coalesced, so a transmitted volume command is paired with the most recent
preceding volume event.

Run its host tests with:

```sh
python3 -m unittest discover -s tools -p 'test_*.py' -v
```

The trace image is not the final release image. Restore and verify the exact
product build after timing or endurance tests.

## M4 diagnostic console isolation

The historical single-character semantic-audio USB console is compiled only
when `CONFIG_SC_ENABLE_SEMANTIC_CONSOLE=y`. It is enabled by
`sdkconfig.m4.defaults` for isolated protocol bring-up and is deliberately
disabled in product, timing and endurance profiles. Do not enable it in a
release image: arbitrary host serial input must never be interpreted as a
product audio command.
