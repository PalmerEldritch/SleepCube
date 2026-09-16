# SleepCube Documentation

## Active documentation scope

The active specification baseline in this repository defines the **SleepCube embedded software system** and the reference hardware interfaces required to run and verify that software.

The active documents are in [`docs/software/`](software/):

1. `00_README.md` — document set, authority and terminology
2. `01_PRD.md` — product/software intent and user-visible outcomes
3. `02_SRS.md` — normative software requirements
4. `03_UX.md` — user interaction and display behaviour
5. `04_SAS.md` — software architecture and ownership boundaries
6. `05_ICD.md` — inter-controller communication contract
7. `06_VVM.md` — verification and validation matrix
8. `07_IMP.md` — implementation and release plan
9. `08_STATUS.md` — authoritative current development state and session handoff

Active architecture decisions are in [`docs/software/adr/`](software/adr/).

`08_STATUS.md` is intentionally separate from the implementation plan: `07_IMP.md` defines what is planned and the milestone gates, while `08_STATUS.md` records what is currently implemented, verified, blocked and recommended next. Development sessions shall keep the status snapshot current according to `AGENTS.md` so continuation does not depend on chat history.

## Legacy prototype/product documentation

Superseded product/prototype requirements, compliance material, implementation notes, process documents and obsolete architecture decisions are retained under [`docs/legacy/`](legacy/) as historical/reference material. They are **not authoritative for the software R00 baseline unless explicitly referenced by an active software document or accepted ADR**.

Examples include:

- `legacy/01_FRS.md`
- `legacy/02_TRS.md`
- `legacy/05_ComplianceMatrix.md`
- `legacy/implementation/SC_P0_*`
- `legacy/adr/ADR-0001-audio-p0-pipeline.md`

These records remain useful evidence of prior requirements, implementation behaviour and decisions, but future software development shall be driven by the active software document set above.

Hardware examples, pin maps, datasheets and vendor/reference source material are kept separately under `/references/` because they may still be technically relevant without being normative software requirements.
