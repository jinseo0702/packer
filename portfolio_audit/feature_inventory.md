# Feature inventory and functional scope

`IMPLEMENTED` means a complete code path is present; it does not claim runtime
correctness. No entry below is runtime-verified at CHECKPOINT A.

## Actual packer features

| Feature | Status | Source evidence | Runtime verified? |
|---|---|---|---|
| Single input path with optional user key | IMPLEMENTED | `src/arg.c:55-75` | No |
| Exactly 16 hexadecimal digits, optional `0x`/`0X` prefix | IMPLEMENTED | `src/arg.c:14-37` | No |
| Random 64-bit key when omitted | IMPLEMENTED | `src/arg.c:40-52` | No |
| Non-destructive private mapping of input | IMPLEMENTED | `src/io_unit.c:3-33` | No |
| ELF magic validation | IMPLEMENTED | `src/format_router.c:3-10` | No |
| ELF32 little-endian i386 `ET_EXEC`/`ET_DYN` admission | IMPLEMENTED | `src/elf_parser.c:3-61` | No |
| ELF64 little-endian x86-64 `ET_EXEC`/`ET_DYN` admission | IMPLEMENTED | `src/elf_parser.c:3-40,64-105` | No |
| Program-header table bounds and entry-size validation | IMPLEMENTED | `src/elf_parser.c:42-83` | No |
| Eligible `PT_LOAD` discovery and metadata capture | IMPLEMENTED | `src/check_meta.c:17-127` | No |
| Existing executable segment page-tail selection | IMPLEMENTED | `src/check_meta.c:129-178` | No |
| XOR transform of selected ELF32/ELF64 file ranges | IMPLEMENTED | `src/encryption_xor.c:3-55` | No |
| ELF32/ELF64 decoder payload selection | IMPLEMENTED | `src/shellcode_data.c:99-124` | No |
| Runtime metadata/marker payload construction | IMPLEMENTED | `src/enter_data.c:3-85` | No |
| Fixed `woody` executable output creation | IMPLEMENTED | `src/write_output.c:101-141` | No |
| Entry-point patch | IMPLEMENTED | `src/write_output.c:19-40,119-125` | No |
| Stub segment `p_filesz`/`p_memsz` extension | IMPLEMENTED | `src/write_output.c:43-99,126-130` | No |
| Runtime permission change, decode, permission restore, marker, original-entry return | IMPLEMENTED | `src/stub.s:250-343` | No |
| Central mapped/heap resource cleanup | IMPLEMENTED | `src/woody-packer.c:3-24,45-83`; `src/io_unit.c:36-50` | No |
| Configurable output path | NOT IMPLEMENTED | output name is literal `woody` at `src/write_output.c:113` | No |
| Big-endian ELF | NOT IMPLEMENTED | rejected at `src/elf_parser.c:12-14` | No |
| Architectures other than i386/x86-64 | NOT IMPLEMENTED | rejected at `src/elf_parser.c:35-38` | No |
| ELF types other than `ET_EXEC`/`ET_DYN` | NOT IMPLEMENTED | rejected at `src/elf_parser.c:33-34` | No |
| New-section/new-segment insertion | NOT IMPLEMENTED | writer only patches existing entry/PHDR and writes into an existing segment tail | No |

## PROFILE B symbol-facing inventory

| Feature | Status | Source evidence | Runtime verified? |
|---|---|---|---|
| ELF section-header parsing | NOT IMPLEMENTED | no `Elf*_Shdr` use in `src/` or `include/` | No |
| Symbol-table (`SHT_SYMTAB`/`SHT_DYNSYM`) parsing | NOT IMPLEMENTED | no symbol/section parser in target source | No |
| Symbol-name extraction | NOT IMPLEMENTED | no string-table traversal in target source | No |
| GNU nm-like symbol type classification | NOT IMPLEMENTED | no `Elf*_Sym` use or classification table | No |
| Symbol inclusion/exclusion options | NOT IMPLEMENTED | CLI accepts only path and optional key | No |
| Symbol sorting options | NOT IMPLEMENTED | no symbol collection or sorting path | No |
| Preservation of GNU nm-observed symbols after packing | UNKNOWN | writer begins with a full input copy, but selected load bytes are modified; paired oracle test required | No |
| Relocatable object (`ET_REL`) input | NOT IMPLEMENTED | admission rejects types other than `ET_EXEC`/`ET_DYN` | No |
| Static archive input | NOT IMPLEMENTED | archive magic fails the ELF gate | No |
| Shared object (`ET_DYN`) end-to-end packing | PARTIAL | admitted by ELF parser; later layout/stub-space suitability and runtime result are unverified | No |
| Stripped executable input | UNKNOWN | no dependency on section/symbol tables is visible, but end-to-end result is untested | No |

PROFILE B's direct comparison contract is therefore absent: the program does not
produce a symbol listing. The planned comparison uses GNU nm on the original and
packed ELF as a preservation check.

## Current functional scope

The following counts describe code admission or representation, not semantic
correctness:

| Dimension | Measured scope | Basis |
|---|---:|---|
| ELF classes admitted | 2 (`ELFCLASS32`, `ELFCLASS64`) | code branches |
| Byte orders admitted | 1 (little-endian) | code gate |
| ELF types admitted | 2 (`ET_EXEC`, `ET_DYN`) | code gate |
| Machine/class pairs admitted | 2 (i386/ELF32, x86-64/ELF64) | code gate |
| Decoder payloads embedded | 2 | code arrays / binary fixtures |
| Decoder sizes | 448 bytes (32-bit), 592 bytes (64-bit) | file size and code array |
| Key width | 64 bits | `uint64_t` and 16 hex digits |
| Key acquisition modes | 2 (explicit, `/dev/urandom`) | argument path |
| Candidate segment flag masks | 3 (`R`, `R|X`, `R|W|X`) | boolean predicate in `fill_arrays_*` |
| Output names | 1 (`woody`) | literal output path |
| Payload fixed overhead | stub size + 48 bytes | 16-byte marker + four 8-byte fields |
| Payload per selected segment | 24 bytes | three serialized 64-bit fields |
| nm-style features implemented | 0 | no section/symbol/output path |

The actual number of encrypted segments and whether a suitable injection region
exists are input-dependent and remain `UNKNOWN` until fixtures are executed.
