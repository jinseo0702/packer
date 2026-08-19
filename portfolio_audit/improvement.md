# Improvement candidate selection

No source change has been made.

## Candidate comparison

| Candidate | Portfolio value | Correctness impact | Architectural interest | Implementation cost | Evidence state |
|---|---|---|---|---|---|
| Explicitly distinguish executable `ET_DYN` from shared objects and reject unsupported `.so` | High | High for accepted-input correctness | High: support policy and loader semantics | Low–medium | E15 CRASH verified |
| Add a native shared-object decode/initialization route | High | High if fully correct | Very high: constructors, relocations, RELRO, loader order | Very high | Need demonstrated; no design yet |
| Replace page-tail dependency with a more general insertion architecture | High | Potentially high | High: ELF layout rewriting | High | Padding limitation verified from code; no runtime failure measured here |

Qualitative levels are used rather than invented numeric scores.

## Recommended first improvement

Introduce an explicit supported-input classifier and reject shared objects before
any transformation. Preserve current paths only for executable formats whose entry
stub is guaranteed by the declared support policy.

Why this candidate comes first:

- It directly converts a verified success-then-CRASH behavior into honest scope
  handling.
- It addresses the root cause at the format admission boundary rather than masking
  the signal.
- It is smaller and easier to regression-test than implementing shared-library
  initialization semantics.
- It creates a strong portfolio case: structural checks alone passed, behavioral
  testing exposed a loader-semantics mismatch, and the correction would make the
  support contract explicit.

## Required approval and regression gate

If approved, the change should be developed on a separate branch/state and must:

1. add deterministic PIE-versus-shared classification fixtures;
2. make E15 a graceful expected rejection rather than a CRASH;
3. rerun all 23 adapted PROFILE B and 22 PROFILE E baseline cases;
4. preserve the existing executable PASS cases;
5. record source hashes and Before/After classifications.
