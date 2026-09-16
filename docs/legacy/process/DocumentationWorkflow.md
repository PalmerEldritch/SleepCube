# Documentation Workflow

## Goals

- Keep API comments and implementation docs synchronized.
- Make promotion from "working code" to "documented behavior" explicit.
- Minimize overhead during active development.

## Two-Level Documentation

1. API-Level (Doxygen in headers)
- Mandatory for public functions.
- Focus on contract: inputs, outputs, side effects, errors.

2. Functional-Level (Markdown in `docs/implementation`)
- Mandatory for stable subsystem behavior.
- Focus on runtime flow and design intent.

## Maturity Signal (Proposed Team Convention)

Use `@docready` in public API Doxygen comments when behavior is stable enough
to be described as implementation intent.

Then update one row in `docs/implementation/DocQueue.md`:

- `Status`: `Candidate` -> `Documented` -> `Needs Update`
- `Notes`: Link to implementation document section

## Suggested Definition of Done

For each non-trivial feature:

1. Code compiles and basic verification passes.
2. Public API comments are updated.
3. If marked `@docready`, implementation docs are updated in the same change.
4. If architecture changed, add/update an ADR.

## Review Checklist

- Are all new/changed public APIs documented?
- Do docs reflect current runtime behavior?
- Are limitations and assumptions explicitly stated?
- Is there any `@docready` API missing from `DocQueue.md`?
