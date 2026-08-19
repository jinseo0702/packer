# Repository architecture audit

All statements in this document are `VERIFIED FROM CODE` unless explicitly marked
otherwise.

## End-to-end control flow

```text
CLI: <ELF path> [16-hex-digit key]
  ↓
argument/key handling (explicit key or /dev/urandom)
  ↓
read-only open → size discovery → writable MAP_PRIVATE mapping
  ↓
ELF magic, class, endianness, type, machine, PHDR-table validation
  ↓
PT_LOAD scan and encryption-metadata construction
  ↓
select executable segment with page-tail space for the runtime payload
  ↓
XOR selected file-backed load-segment bytes in the private mapping
  ↓
build payload: decoder stub + 16-byte marker + fixed metadata + segment records
  ↓
copy mapping to ./woody; patch e_entry and one existing program header;
write payload at the selected segment's former file end
  ↓
packed program starts at stub → derives load base → mprotects/decrypts segments
→ restores protection → prints marker → returns to original entry
```

Source path: `src/woody-packer.c:27-84`, with stage implementations listed below.

## Components

| Component | Responsibility | Source evidence |
|---|---|---|
| Entry point / coordinator | Orders every stage and centralizes cleanup on error | `src/woody-packer.c:3-84` |
| Argument/key parser | Accepts one path and optional exactly-16-digit hexadecimal key; otherwise reads 64 bits from `/dev/urandom` | `src/arg.c:14-75` |
| I/O unit | Opens input read-only and maps it `PROT_READ|PROT_WRITE, MAP_PRIVATE`; unmaps/closes on destruction | `src/io_unit.c:3-50` |
| Format gate | Checks only ELF magic at this stage | `src/format_router.c:3-10` |
| ELF parser | Selects 32/64-bit view; accepts little-endian `ET_EXEC`/`ET_DYN` for i386/x86-64; bounds-checks PHDR table | `src/elf_parser.c:3-105` |
| Segment policy | Collects eligible `PT_LOAD` records and finds an executable segment with sufficient page-tail space | `src/check_meta.c:17-217` |
| Transformer | XORs eligible file-backed segment ranges in the private image | `src/encryption_xor.c:3-55` |
| Payload builder | Serializes stub, marker, original/new entry, key, count, and 24-byte segment records | `src/enter_data.c:3-85` |
| ELF writer | Copies the image, patches `e_entry`, extends one PHDR's `p_filesz/p_memsz`, and inserts payload | `src/write_output.c:3-141` |
| Runtime decoder | Computes runtime base, changes page permissions when needed, reverses XOR, restores permissions, writes marker, returns | `src/stub.s:250-343` |
| Embedded stubs | Provides 448-byte ELF32 and 592-byte ELF64 decoder byte arrays | `src/shellcode_data.c:3-124`; binary sizes independently inspected |

## Principal data structures

| Structure | Meaning | Evidence |
|---|---|---|
| `t_unit` | Mapped file base/limit, key, class-specific ELF/PHDR pointers, fd, ELF class | `include/woody.h:47-64` |
| `t_encryption` | Per-selected-segment file offset, file size, virtual address, PHDR index, stub flag | `include/woody.h:66-72` |
| `t_meta` | Runtime virtual address, memory size, and original protection bits | `include/woody.h:73-77` |

## External APIs and lifecycle

- Input: `open`, `lseek`, `mmap`; the fd is closed immediately after mapping.
- Key: `open/read/close` on `/dev/urandom` when no explicit key is supplied.
- Output: `open`, repeated `write`, `lseek`, `close` to fixed file `woody`.
- Runtime stub: direct `mprotect`, `write`, and error-path `exit` system calls for
  each architecture.
- Heap allocations for encryption records, metadata, and payload are released by
  `cleanup_all`; mapping is released by `destroy_unit`.

## Important design characteristics

1. **Program-header-driven transformation.** No section-header, string-table, or
   symbol-table parser exists. Decisions are based on ELF header and `PT_LOAD`
   program headers (`src/elf_parser.c`, `src/check_meta.c`).
2. **Original input protection.** The input is opened read-only and transformed in
   a private mapping, so intended writes do not propagate to the input file
   (`src/io_unit.c:7-32`).
3. **No new segment or section.** The payload occupies page-tail capacity following
   an existing executable load segment; its `p_filesz`/`p_memsz` and the ELF entry
   are patched (`src/check_meta.c:140-177`, `src/write_output.c:43-140`).
4. **Header-bearing segment excluded.** Only `PT_LOAD` entries with nonzero file
   offset are candidates, keeping the offset-zero ELF/PHDR region unencrypted
   (`src/check_meta.c:17-126`).
5. **Self-describing runtime payload.** The output embeds the original entry, new
   entry, plaintext XOR key, record count, and per-segment metadata after a fixed
   stub/marker layout (`src/enter_data.c:20-53`).
6. **Class-specific transform width.** ELF64 uses 8-byte XOR chunks; ELF32 uses
   4-byte chunks, with single-byte tails using the low key byte
   (`src/encryption_xor.c:3-55`).
7. **Bounds checks are explicit but localized.** `CHECK_RANGE` uses subtraction to
   avoid `offset + size` overflow (`include/woody.h:99-105`), and is used for file
   table/segment ranges. End-to-end malformed-input safety remains `UNKNOWN` until
   runtime tests.

## Architecture-specific behavior

- ELF32 accepts only `EM_386`; ELF64 accepts only `EM_X86_64`.
- Separate 32/64 decoder stubs use `int 0x80` versus `syscall` and 4-byte versus
  8-byte decode strides.
- All serialized payload metadata fields are 64-bit even for ELF32.
- `INFERENCE`: successful runtime decoding depends on the embedded byte arrays
  matching the documented `stub.s` layout; that equivalence has not been proven at
  CHECKPOINT A.
