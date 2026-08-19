# Design rationale

This document deliberately separates repository evidence, user rationale, and
analysis inference.

## Verified facts

- `VERIFIED FROM CODE`: The program accepts little-endian ELF32/i386 and
  ELF64/x86-64 `ET_EXEC`/`ET_DYN`, transforms selected file-backed `PT_LOAD`
  segments, inserts a decoder in an existing executable segment's page-tail, and
  changes the entry point.
- `VERIFIED FROM CODE`: It does not parse or create section headers, add a program
  header, exploit a software vulnerability, or implement an infection/propagation
  path.
- `VERIFIED FROM RUNTIME TEST`: Under adapted PROFILE B, GNU nm-observed symbol
  metadata was preserved in all 23 focused cases; executable behavior was not part
  of that baseline.

## User-provided rationale

- `USER-PROVIDED RATIONALE`: The project began after learning that viruses enter
  through vulnerabilities; the user wanted to implement code directly and observe
  how vulnerabilities can exist.
- `USER-PROVIDED RATIONALE`: The existing executable-segment padding approach was
  chosen to avoid loader errors and to make a simple `readelf -h` header view less
  revealing of the transformation.
- `USER-PROVIDED RATIONALE`: Encryption was limited to segments because encrypting
  other regions would increase implementation complexity.
- `USER-PROVIDED RATIONALE`: If redesigned, the project would be structured more
  systematically for extension. The user identifies lack of injection padding and
  easy detection caused by simple, visible encryption as current limitations, and
  believes a redesign should reconsider the approach in light of more sophisticated
  real-world attacks.

## Inference and boundaries

- `INFERENCE`: The project is best evidenced as an exploration of ELF layout,
  loader constraints, self-modifying runtime behavior, and the observability of
  binary transformations—not as evidence that it discovers or exploits a concrete
  vulnerability.
- `INFERENCE`: Avoiding new ELF structures reduces implementation surface but makes
  success dependent on available space in a suitable existing segment.
- `INFERENCE`: A plain header-only inspection may not explain encrypted byte ranges,
  but `readelf -h` is not actually bypassed: the output remains an ELF and its entry
  change remains visible. Whether the whole transformation evades detection is
  unmeasured and must not be claimed.
- `INFERENCE`: “More systematic and extensible” is not yet an architecture decision.
  It would need explicit module boundaries, supported-format policy, invariants, and
  tests before it can be presented as a verified redesign.
