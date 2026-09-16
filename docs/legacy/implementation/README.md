# Implementation Documentation

This folder contains functional implementation descriptions for firmware behavior.

## Purpose

- Explain how each subsystem works at runtime.
- Capture boundaries, assumptions, and limitations.
- Keep implementation intent aligned with code changes.

## Files

- `TEMPLATE.md`: Base template for new subsystem documents.
- `DocQueue.md`: Promotion queue for documentation maturity.
- `SC_P0_AudioPlayback.md`: PoC audio decode/playback and service controls.
- `SC_P0_LightingEngine.md`: LED backend, animation layers, smoothing pipeline.
- `SC_P0_RuntimeServices.md`: App-core event routing, service startup, temporary button UI.
- `SC_P0_RTOSTaskSchedule.md`: Simplified FreeRTOS task/priority/switching model.
- `SC_P0_TaskTiming.md`: Generated task timing diagram from runtime trace logs.

## Rule

When a function or module is considered stable enough for long-lived behavior documentation,
mark the API comment with `@docready` and add/update one row in `DocQueue.md`.
