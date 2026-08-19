# Failure analysis — adapted PROFILE B CHECKPOINT B

## Classification result

| Classification | Count | Evidence |
|---|---:|---|
| PASS | 23 | VERIFIED FROM RUNTIME TEST |
| PARTIAL | 0 | VERIFIED FROM RUNTIME TEST |
| FAIL | 0 | VERIFIED FROM RUNTIME TEST |
| CRASH | 0 | VERIFIED FROM RUNTIME TEST |
| SETUP_FAILURE | 0 | VERIFIED FROM RUNTIME TEST |

Source of record: `test_results.json`. Raw commands, stdout, stderr, exit status,
hashes, and structural inspection are under `raw/<case-id>/`.

There are no qualifying FAIL or PARTIAL cases to analyze. Creating three to five
failure narratives would fabricate evidence, contrary to the audit rules.

## Most important PASS cases

| Case | Verified result |
|---|---|
| B03 — ELF64 ET_EXEC | Output created; all 40 GNU nm POSIX records matched as a multiset |
| B04 — ELF64 PIE | Output created; all 43 records matched |
| B05 — ELF32 ET_EXEC | Output created; all 10 records matched |
| B06 — tracked ELF32 PIE | Output created; all 34 records matched |
| B07 — stripped ELF64 | Both files produced the same zero-symbol result and diagnostic semantics |
| B12 — numeric ordering | Ordered sequence of 40 `nm -n` records matched exactly |
| B13 — global view | All 24 `nm -g` records matched |
| B14 — shared object | Output created; all 25 records matched |
| B15–B22 | Unsupported/malformed/layout inputs followed their expected graceful rejection path |

## Correctness versus scope limitation

### Verified correctness within adapted PROFILE B

- `VERIFIED FROM RUNTIME TEST`: Symbol name, value, type, multiplicity, selected
  inclusion, and tested ordering were preserved in all admitted paired comparisons.
- `VERIFIED FROM RUNTIME TEST`: Expected unsupported/malformed inputs returned
  nonzero, did not crash or time out, and did not create `woody`.
- `VERIFIED FROM RUNTIME TEST`: Five synthetic comparator corruptions were detected.

### Scope limitations, not baseline failures

- `VERIFIED FROM CODE`: The target has no section/symbol parser or GNU nm-like
  listing interface. PROFILE B can only observe transformed files externally.
- `VERIFIED FROM RUNTIME TEST`: `ET_REL`, archives, big-endian ELF, unsupported
  machine values, invalid PHDR bounds, and a layout without stub space are rejected
  as expected; these PASS cases do not demonstrate support for those inputs.
- `VERIFIED FROM CODE`: Only little-endian ELF32/i386 and ELF64/x86-64
  `ET_EXEC`/`ET_DYN` are admitted.

### Still unknown

- `UNKNOWN`: Whether generated `woody` files execute successfully.
- `UNKNOWN`: Whether stdout, stderr, exit status, arguments, and stateful behavior
  match their original executables.
- `UNKNOWN`: Whether entry-point/PHDR/payload changes satisfy all structural
  invariants across supported layouts.
- `UNKNOWN`: Whether the embedded decoder byte arrays exactly correspond to and
  behave like the checked-in `stub.s` source on every supported class.

These are the packer's central correctness questions, but they are outside the
approved adapted PROFILE B plan.

## Portfolio failure candidate

None is evidence-backed under this baseline. The most valuable next investigation
would be execution/structure preservation under the packer-specific PROFILE E, but
that is a new audit scope rather than a fix to a demonstrated PROFILE B failure.

## Possible next validation architectures

These are `INFERENCE` and require user approval; they are not proposed source fixes.

1. **Paired behavioral oracle:** run the original and `woody` with identical argv,
   stdin, environment, and timeout; compare stdout, stderr, exit status, and signal.
2. **Structural delta oracle:** capture readelf/objdump models before and after;
   permit only the intended entry point, selected PHDR sizes, encrypted ranges, and
   payload region to differ.
3. **Fixture matrix:** cross class (32/64), type (EXEC/PIE/shared where admitted),
   stripped state, dynamic/static linkage, and segment layout, then classify
   behavioral and structural results independently.

## Approved PROFILE E extension

The subsequent PROFILE E extension executed that behavioral/structural matrix and
found one evidence-backed CRASH: an admitted shared object is structurally
transformed successfully but raises `SIGILL` when loaded through `dlopen`.

See [profile_e_failures.md](profile_e_failures.md) for expected/actual behavior,
signal address, code path, root cause, severity, and fix directions.
