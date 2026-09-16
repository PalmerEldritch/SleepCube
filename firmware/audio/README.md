# SleepCube Audio Controller

Standalone ESP-IDF firmware for the AtomS3 Lite and ATOMIC Speaker Base.

M1 is a generated-tone hardware diagnostic only. It intentionally contains no
microSD, codec, UART, session-controller, or Waveshare dependency.

## Toolchain

- ESP-IDF v6.0, commit `662a3be354759d9487bf4b1a629fadb766cb1800`
- target: `esp32s3`
- configured flash size: 8 MB

Source the ESP-IDF environment before using `idf.py`. On the current
development machine:

```sh
source /home/jeppe/esp/v6.0/esp-idf/export.sh
```

## Clean build

From this directory:

```sh
idf.py fullclean
idf.py set-target esp32s3
idf.py build
```

For a clean build without creating project-local configuration or build files:

```sh
idf.py -B /tmp/sleepcube-audio-build \
  -D SDKCONFIG=/tmp/sleepcube-audio-sdkconfig \
  set-target esp32s3 build
```

## Flash and monitor

Connect the AtomS3 Lite over USB-C. Flashing requires two manual reset-button
actions:

1. Before starting the flash command, hold reset for about two seconds until
   the internal green LED lights, then release it to enter download mode.
2. Run the flash command and wait until it reports completion.
3. Press reset once after flashing completes to leave download mode and start
   the application.

```sh
idf.py -p /dev/ttyACM0 flash
```

The application USB serial interface may re-enumerate under a new device name.
Attach the monitor to the application-mode device without resetting it:

```sh
idf.py -p /dev/ttyACM1 monitor --no-reset
```

Exit the monitor with `Ctrl-]`. Device names are examples; identify the actual
port before each command. Do not combine `flash monitor` for this board because
the manual post-flash reset must occur between those operations.

## M1 diagnostic operation

Boot is silent. The firmware waits two seconds for the native USB Serial/JTAG
endpoint to re-enumerate, then logs its identity, detected flash size, I2S
configuration, and `READY`. Press the AtomS3 Lite button once to play one finite
sequence:

1. 500 ms silence;
2. 440 Hz tone with 250 ms fade-in, 2 s steady level, and 250 ms fade-out;
3. 500 ms silence;
4. 1 kHz tone with the same envelope;
5. 500 ms silence.

The M1 peak level is approximately -24 dBFS. Completion logs report the
sequence number and exact frame/byte counts: `286650` frames and `1146600` bytes for a
complete sequence. Any I2S error disables output and latches a silent fault
state until reboot.

The retained M1 configuration is 44.1 kHz signed 16-bit standard
Philips-I2S stereo. Each generated mono sample is duplicated into both channel
slots so the NS4168 channel selection cannot discard the signal.

| Signal | AtomS3 Lite GPIO |
| --- | --- |
| BCLK | GPIO5 |
| LRCLK/WS | GPIO39 |
| DATA | GPIO38 |
| MCLK | Not used |

This bring-up format is not the final R00 content-format decision.
