# ADR-0003 — Two-controller software architecture

**Status:** Accepted  
**Date:** 2026-09-08

## Context

The original SleepCube prototype placed product logic, touch UI, lighting and audio playback on the Waveshare ESP32-C6 controller. The UI and lighting implementation matured successfully, but the board-specific audio path remained problematic and accumulated significant diagnostic and workaround complexity.

A M5Stack AtomS3 Lite and ATOMIC Speaker Base (NS4168) have been selected as a modular audio platform.

## Decision

SleepCube software shall use two cooperating embedded controllers:

- **Control Controller:** Waveshare ESP32-C6-Touch-LCD-1.47. Owns overall product/session state, touch UI, display, RGB lighting, user interaction and cross-domain coordination.
- **Audio Controller:** M5Stack AtomS3 Lite + ATOMIC Speaker Base. Owns audio storage access, decoding, buffering, gain/fades, I2S output, authoritative playback state and independent enforcement of finite playback timeout.

The existing Waveshare UI and lighting implementation shall be retained and migrated rather than rewritten.

The Control Controller shall treat audio as a service behind an application-facing abstraction. It shall not control the Audio Controller's decoder, filesystem or I2S implementation directly.

## Consequences

### Positive

- audio faults and timing load are isolated from UI/lighting;
- working Waveshare UI/lighting code can be preserved;
- audio hardware can be developed and validated independently;
- each controller has a clear ownership boundary;
- replacement of either hardware platform is possible without redesigning the entire software product.

### Costs

- two firmware images must be built, flashed and versioned;
- an inter-controller protocol and synchronization model are required;
- distributed state introduces explicit recovery/reconciliation cases;
- hardware requires a wired controller-to-controller connection and common ground.

## Rejected alternatives

### Continue debugging local Waveshare audio

Rejected because repeated investigation did not achieve acceptable audio quality and was increasing board-specific complexity without improving the product architecture.

### Move all SleepCube functions to AtomS3

Rejected because the existing Waveshare board already provides the required display/touch platform and has mature UI/lighting software worth retaining.

### Split product state equally across controllers

Rejected because ambiguous ownership would increase synchronization complexity. Overall product/UI state remains Control-owned; audio execution state remains Audio-owned.
