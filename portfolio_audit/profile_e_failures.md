# PROFILE E failure analysis

## E15 — accepted shared object crashes when loaded

Classification: `CRASH` and correctness bug at the admitted support boundary.

### Expected

`VERIFIED FROM RUNTIME TEST`: Loading the original `ET_DYN` shared object with
`dlopen`, resolving `shared_exported_function`, and calling it returns exit status
0 and stdout `shared-result:78\n`.

### Actual

`VERIFIED FROM RUNTIME TEST`:

- The packer returns 0 and creates `woody`.
- All nine structural checks pass: intended header/PHDR delta, XOR ranges, payload,
  metadata, load invariants, and byte-delta allowlist.
- Loading the packed shared object terminates the loader with signal 4 (`SIGILL`),
  without stdout or stderr.
- Unsandboxed `strace` maps the packed RX segment at `0x7fbd4e113000` and reports
  `SIGILL` at `0x7fbd4e113004`, module-relative address `0x1004`.
- The original `.init` at `0x1000` contains valid startup instructions. In the
  packed file those bytes are XOR-transformed; objdump shows invalid instructions
  beginning at `.init+4`, matching the signal address.

Evidence: `raw/profile_e/E15/`.

### Relevant code path

1. `src/elf_parser.c:18-39` admits every matching-architecture `ET_DYN`; it does not
   distinguish a PIE executable from a shared object.
2. `src/check_meta.c:95-127` selects the shared object's file-backed `R|X` and `R`
   load segments.
3. `src/woody-packer.c:59-80` encrypts those ranges, builds the payload, and writes
   a successful output.
4. `src/write_output.c:101-140` changes `e_entry` from `0` to `0x11b1` and extends
   the selected RX program header.
5. `src/stub.s:277-328` can decrypt the segments only after control reaches the new
   entry stub.

### Root cause

`VERIFIED FROM CODE` plus `VERIFIED FROM RUNTIME TEST`: The transform assumes the
ELF header entry point runs before any encrypted code. That is valid for the tested
executables, but `dlopen` does not start a shared object through `e_entry`; the
dynamic loader executes its initialization path. Because `.init` was encrypted and
the decoder entry never ran, the loader executes ciphertext and raises `SIGILL`.

The immediate defect is over-broad `ET_DYN` admission, not the XOR calculation or
payload serialization: those structural checks pass exactly.

### Severity

High for shared-object inputs: the tool reports success and creates an unusable
artifact. Scoped for the measured executable inputs: ELF32/64 ET_EXEC, PIE, static,
and stripped behavioral cases passed.

### Possible fix directions

1. **Recommended: explicit executable-only admission.** Define the supported
   `ET_DYN` executable signature and reject shared objects before transformation.
   Candidate evidence includes `PT_INTERP` and PIE dynamic flags, but the exact
   policy must cover or deliberately exclude static PIE and unusual loaders.
2. **Explicit support matrix/router.** Split validation into ET_EXEC, dynamic PIE,
   static PIE, and shared-object routes, each with its own preconditions and tests.
   Initially only executable routes need proceed to the current entry-stub design.
3. **Native shared-object initialization route.** A shared-library-capable design
   would need decoding before any encrypted initializer or exported function runs,
   with relocation/constructor/RELRO semantics handled explicitly. This is much
   higher cost and is not recommended as the first correction.

No source fix has been applied.

## Other PROFILE E classifications

No additional FAIL, PARTIAL, or CRASH cases were observed. Reporting only one
failure is intentional; inventing two more narratives would violate the evidence
rules.
