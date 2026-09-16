# ADR-0004 — UART semantic audio link and independent timeout

**Status:** Accepted  
**Date:** 2026-09-08

## Context

The two-controller architecture requires a local wired interface between the Waveshare Control Controller and AtomS3 Audio Controller. Available dedicated GPIO permit a simple point-to-point link. The application needs only low-bandwidth control/status exchange; audio samples remain local to the Audio Controller.

A key product invariant is that audio playback is always finite.

## Decision

The controllers shall communicate over **3.3 V full-duplex UART**, nominally 115200 8N1, using dedicated GPIO and common ground.

The protocol shall expose **semantic audio operations** such as PLAY, STOP, SET_VOLUME and GET_STATUS rather than low-level remote filesystem, decoder, buffer or I2S control.

The Audio Controller shall be authoritative for actual playback state and shall independently enforce a finite playback deadline for every successful PLAY operation.

The Control Controller shall own user-visible session state and presentation, but loss or reset of the Control Controller shall not remove an already-established Audio Controller playback deadline.

The exact wire framing, integrity/checksum mechanism and acknowledgement/idempotency strategy are deferred to a later ADR during the UART/protocol milestones.

## Consequences

### Positive

- uses only two signal GPIO plus ground;
- simple to inspect with terminal/logic-analyzer tooling;
- full-duplex status/events are straightforward;
- protocol bandwidth is far above application need;
- audio timing/data stays isolated on the Audio Controller;
- finite-session safety does not depend on continued Control Controller operation.

### Costs

- protocol framing and resynchronization must be designed explicitly;
- distributed commands require acknowledgement/state reconciliation;
- protocol compatibility/versioning must be maintained across two firmware images.

## Rejected alternatives

### I2C

Rejected because master/slave semantics add coupling without benefit for this point-to-point bidirectional command/status link.

### SPI

Rejected because higher bandwidth is unnecessary and would consume more signals and framing complexity.

### Wi-Fi/BLE

Rejected because wireless connectivity is unnecessary for the product function and would add provisioning, software and fault complexity.

### Audio/control multiplexing over I2S or custom one-wire protocol

Rejected because it couples unrelated timing domains or introduces custom transport risk merely to save GPIO that are already available.
