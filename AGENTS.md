# SleepCube agent instructions

## Scope
This repo contains both source code and engineering documentation.
Documentation is in /docs and is written in Markdown.

## Writing rules for requirements
- Use Markdown tables for requirements. One requirement per row.
- Use "shall" statements for requirements.
- Keep FRS implementation-agnostic (no components, no circuits, no specific ICs).
- TRS may introduce technical constraints but avoid premature design choices unless explicitly requested.
- Never invent standards clauses or compliance references. If unknown, write "TBD" and note what input is needed.

## IDs and traceability
- FRS IDs: FR-<AREA>-### (e.g., FR-AUD-001)
- TRS IDs: TR-<AREA>-### derived from one or more FR IDs.
- Every TR must reference at least one FR in the "Derived From" column.
- Prefer consistent terminology; when unsure, consult docs/00_Glossary.md.

## Output expectations
- When asked to create/modify docs, preserve table structure and IDs.
- When asked for a matrix, ensure all FR IDs are represented and flag missing mappings.

## Conversation triggers
- Trigger phrase: `promote: <scope>`
- Meaning: The user signals that functionality is mature enough for functional implementation documentation.
- Required actions:
  - Update relevant file(s) in `/docs/implementation/`.
  - Update `/docs/implementation/DocQueue.md` status and notes.
  - Update Doxygen comments in public headers if API contracts changed.
  - Add or update ADR in `/docs/adr/` when architectural decisions are introduced or changed.
