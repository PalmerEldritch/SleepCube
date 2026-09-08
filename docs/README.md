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

Architecture decisions remain in [`docs/adr/`](adr/).

## Legacy prototype/product documentation

The existing FRS, TRS, compliance matrix, prototype BOM, implementation notes, datasheets and hardware investigation material are retained as historical/reference material. They describe earlier prototype and whole-product development and are **not authoritative for the software R00 baseline unless explicitly referenced by an active software document or ADR**.

In particular:

- `01_FRS.md`
- `02_TRS.md`
- `05_ComplianceMatrix.md`
- `implementation/SC_P0_*`

remain useful evidence of prior decisions and implementation behaviour, but future software development shall be driven by the active software document set above.
