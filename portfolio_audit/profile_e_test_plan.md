# PROFILE E extension — behavioral and structural baseline

Status: approved after the adapted PROFILE B CHECKPOINT B.

## Goal

Verify that a successful transform makes only its intended ELF layout changes and
preserves the original program's behavior within the code-admitted support range.

## Oracles

1. Exact original executable behavior under the same argv, stdin, environment,
   working directory, and timeout.
2. A class-aware ELF parser in the harness plus GNU readelf/objdump/file raw output.
3. Code-derived transformation invariants: selected `PT_LOAD` ranges, XOR rule,
   payload layout, new entry point, and one extended program header.

## Behavioral normalization

Normalized:

- Both programs receive the same synthetic `argv[0]`, independent of their paths.
- Exactly one leading 14-byte `....WOODY....\n` marker is removed from packed
  stdout before semantic comparison because the marker is an explicit target
  feature (`src/stub.s:324-328`). The raw output is always retained.

Not normalized:

- Program stdout after that single marker, stderr, exit status, signal, timeout,
  argv contents, stdin bytes, environment-derived output, or stateful effects.
- Missing, duplicated, displaced, or modified marker bytes.

## Structural invariants

- ELF class, byte order, type, machine, table locations/counts, and section-header
  metadata remain unchanged; only `e_entry` may change in the ELF header.
- Program-header count and all fields remain unchanged except one selected
  executable load segment's `p_filesz` and `p_memsz`, each increased by the exact
  payload size.
- New entry equals selected segment `p_vaddr + original p_filesz` and lies in the
  extended executable segment.
- Eligible segment bytes equal the code-derived XOR transform for the supplied key.
- Payload equals class-specific shellcode + 16-byte marker area + four 64-bit fields
  + three 64-bit fields per encrypted segment.
- No byte outside the entry field, selected PHDR size fields, encrypted ranges, and
  payload range may change.
- Original input hash must remain unchanged and output must remain executable ELF.

## Planned cases

Twenty-two focused cases cover ELF64 ET_EXEC/PIE/static/stripped, ELF32 static/PIE,
argv/stdout/stderr/exit/stdin/environment, boundary keys, shared-object loading,
header/PHDR/range/payload invariants, deterministic output, input integrity, and
output mode/format. Shared objects are included because the parser admits all
`ET_DYN` without distinguishing PIE from `.so`.
