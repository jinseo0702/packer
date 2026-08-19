# Baseline test plan — adapted PROFILE B

Status: planned only. No fixture, harness, output file, or target runtime result was
created at CHECKPOINT A.

## Question under test

Because `woody_woodpacker` is an ELF transformer rather than an nm implementation,
PROFILE B is adapted to ask:

> For inputs that the packer accepts, does GNU nm observe the same symbol names,
> values, types, inclusion/exclusion, and ordering before and after packing; and are
> out-of-scope/malformed PROFILE B inputs rejected without a crash?

This does **not** answer whether the packed program preserves stdout, stderr,
arguments, exit status, or general execution behavior.

## Reference and supporting oracles

1. **Primary:** GNU nm 2.46, the installed PROFILE B reference.
2. **Paired baseline:** GNU nm output for the exact original fixture. The expected
   post-pack symbol observation is equality with this paired baseline.
3. **Fixture admission/setup:** `file` 5.46 and GNU readelf 2.46 confirm class, type,
   machine, sections, symbol tables, and intended corruption before the target runs.
4. **Code-derived rejection oracle:** the documented admission gates in
   `src/format_router.c` and `src/elf_parser.c` determine expected rejection class.

GNU nm is not a reference packer. It is an external semantic observer of symbol
metadata in this adapted plan.

## Deterministic fixture policy

- Compile purpose-built, minimal fixtures in `portfolio_audit/` only.
- Fix `LC_ALL=C` and use an explicit key such as `0123456789abcdef`; never use the
  random-key path in deterministic comparisons.
- Build each fixture once, then compare that exact input/output pair. Do not compare
  addresses across separate compilations.
- Give fixtures explicit symbols covering local/global, weak, undefined, text,
  read-only data, initialized data, and BSS where the file type permits.
- Before invoking the target, assert fixture type/class with `readelf` and assert
  expected symbols with GNU nm. A failed setup is `SETUP_FAILURE`, not target FAIL.
- Run each target case in an isolated working directory because output is always
  named `woody`.

## Planned representative cases

| ID | Fixture/behavior | Expected oracle result | Primary dimension |
|---|---|---|---|
| B01 | no argument | exit nonzero; usage-class diagnostic; no `woody` | CLI rejection |
| B02 | valid path plus malformed key | exit nonzero; key-format diagnostic; no `woody` | CLI rejection |
| B03 | ELF64 non-PIE executable with full symbol mix | target produces output; paired GNU nm records equal | names/values/types |
| B04 | ELF64 PIE with full symbol mix | paired GNU nm records equal | ET_DYN executable |
| B05 | ELF32 non-PIE with representative symbols | paired GNU nm records equal | ELF32/i386 |
| B06 | existing tracked ELF32 PIE `hello` | paired GNU nm records equal | ELF32 ET_DYN |
| B07 | stripped ELF64 executable | GNU nm's no-symbol observation/error semantics remain equivalent | stripped input |
| B08 | ELF64 executable with local and global same-stem symbols | no missing/extra or binding/type change | inclusion/type |
| B09 | ELF64 weak-defined and weak-undefined symbols | `W/V/w/v`-class observations preserved | weak symbols |
| B10 | ELF64 undefined imports | undefined names/types preserved | undefined symbols |
| B11 | text/rodata/data/BSS/absolute symbols | value and type fields preserved | symbol types |
| B12 | `nm -n` paired view | exact symbol sequence preserved | ordering |
| B13 | `nm -g` paired view | exact global inclusion set preserved | inclusion/exclusion |
| B14 | shared object (`ET_DYN`) with exported/hidden/local symbols | if packed, paired GNU nm records equal; otherwise classified by rejection path | shared object scope |
| B15 | relocatable object (`ET_REL`) | graceful nonzero rejection; no output | unsupported type |
| B16 | static archive containing two objects | graceful nonzero non-ELF rejection; no output | archive scope |
| B17 | non-ELF text input | graceful nonzero format rejection; no output | malformed input |
| B18 | truncated ELF magic/header | graceful nonzero rejection; no crash/timeout/output | bounds handling |
| B19 | invalid `EI_CLASS` and invalid `EI_DATA` (separate mutations) | correct nonzero rejection for each; no crash | class/endian gates |
| B20 | valid header with unsupported machine | nonzero machine rejection; no output | machine gate |
| B21 | PHDR table outside file bounds | nonzero PHDR rejection; no crash | bounds handling |
| B22 | valid admitted ELF with no eligible or no sufficiently large RX tail | nonzero load/stub-space rejection; no crash | layout limitation |

B19 will be represented as two independent test invocations even though it shares
one row here. The initial suite therefore contains 23 target cases, within the
requested representative 15–30 range.

## Capture and classification

For every case preserve, without overwriting:

- fixture build command and setup status;
- input and output SHA-256;
- target stdout, stderr, exit status, signal, and timeout state;
- raw GNU nm stdout/stderr/exit status for input and output;
- raw `file` and `readelf` inspection;
- normalized comparison records and the exact comparator result.

Classification follows the supplied definitions:

- `PASS`: the planned symbol semantics/rejection behavior match the oracle.
- `PARTIAL`: core symbol records match but a meaningful selected view or diagnostic
  behavior differs.
- `FAIL`: name/value/type/set/order or expected rejection semantics are wrong.
- `CRASH`: signal, timeout/deadlock, or unusable abnormal termination.
- `SETUP_FAILURE`: fixture/reference prerequisites fail; never counted as target FAIL.

## Normalization

Normalized:

- set `LC_ALL=C` for target/reference commands;
- strip only the input filename prefix from GNU nm diagnostics when comparing
  `original-name:` with `woody:`;
- normalize CRLF to LF and ignore terminal-only trailing horizontal whitespace;
- for semantic set comparisons, parse GNU nm POSIX records into explicit
  `(name, type, value, size)` fields and compare as a multiset;
- compare addresses only within the exact original/packed pair, never between
  independently linked fixtures.

Not normalized:

- symbol name, value, size, or type letter;
- missing or extra symbols;
- duplicate-symbol multiplicity;
- inclusion/exclusion under the tested GNU nm option;
- row order in ordering cases (`nm -n` and the default-order paired view);
- GNU nm or target exit status and error category;
- crash, signal, timeout, or failure to create the expected output;
- changes hidden inside corrupted or unparsable symbol/string tables.

## Checkpoint gate

Phase 4 must not begin until the user approves this plan or resolves the PROFILE B
versus packer/PROFILE E mismatch. In particular, no claim about packed executable
behavior will be made from a passing GNU nm preservation matrix alone.
