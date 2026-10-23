# Architecture Decision Records (ADR)

This folder contains active architecture and design decisions for the released
SleepCube R00 baseline and the R01 development variant.

Historical decisions that no longer describe the active architecture belong under `docs/legacy/adr/` and shall not be treated as authoritative for current implementation work.

## Naming

Use:

`ADR-XXXX-short-title.md`

ADR numbers are permanent identifiers. Do not renumber later ADRs when an earlier ADR is moved to legacy or superseded.

## Lifecycle

Supported states:

- `Proposed` — under discussion; not authoritative.
- `Accepted` — active decision and part of the architecture baseline.
- `Superseded` — replaced by a newer ADR; retained for decision history.

Accepted ADRs are append-only decision history in the architectural sense: do not silently reverse an accepted decision by rewriting it. If the decision changes materially, create a new ADR and mark the previous ADR as superseded. Minor corrections, wording clarification and path/reference maintenance are allowed when they do not change the decision itself.

## When an ADR is required

Create or update an ADR when a decision materially affects one or more of the following:

- controller/subsystem ownership;
- inter-controller architecture or protocol strategy;
- state ownership, synchronization or recovery model;
- persistent-data ownership or storage strategy;
- audio codec/decoder/pipeline architecture;
- major dependency or library selection;
- task/concurrency architecture;
- hardware/software interface assumptions;
- major public software contracts;
- other decisions that would be costly or ambiguous to rediscover later.

Routine implementation details that are already constrained by SRS/SAS/ICD and do not alter architecture generally do not require an ADR.

## Active index

| ADR | Status | Decision |
| --- | --- | --- |
| ADR-0002 | Accepted; RGB clauses superseded by ADR-0009 | Touch-first ambient UI on the Control Controller |
| ADR-0003 | Accepted; RGB clauses superseded by ADR-0009 | Two-controller software architecture |
| ADR-0004 | Accepted | UART semantic audio link and independent playback timeout |
| ADR-0005 | Accepted | microSD-hosted 44.1 kHz/16-bit mono PCM WAV content |
| ADR-0006 | Accepted | Audio Controller owns authoritative persisted volume |
| ADR-0007 | Accepted | Bounded COBS/CRC UART framing and sequence correlation |
| ADR-0008 | Accepted | Semantic boot sessions, idempotent commands and bounded retry recovery |
| ADR-0009 | Accepted; GUI-off clause superseded by ADR-0010 | R01 fixed-color COB PWM lighting and power arrangements |
| ADR-0010 | Accepted | Retain a nonzero minimum brightness in the normal GUI |

## Legacy decision history

| ADR | Status | Location |
| --- | --- | --- |
| ADR-0001 | Superseded by ADR-0003 | `docs/legacy/adr/ADR-0001-audio-p0-pipeline.md` |

## Relationship to other documents

- SRS defines required software behaviour.
- SAS defines the current architecture and ownership model.
- ICD defines the stable inter-controller interface contract.
- ADRs explain significant decisions behind those documents.
- `08_STATUS.md` records current implementation and verification state; it does not supersede ADRs.
