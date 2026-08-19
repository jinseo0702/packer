# Baseline environment

Captured: 2026-08-19T18:49:53+09:00

| Field | Recorded value | Evidence |
|---|---|---|
| Repository | `/home/jinseo/duo/packer` | VERIFIED FROM CODE |
| Commit | `f69b1dac0d08d0442a126121867b4592cc88fa66` | VERIFIED FROM CODE (`git rev-parse HEAD`) |
| Branch | `main`, tracking `origin/main` | VERIFIED FROM CODE |
| Dirty before audit files | No; 0 porcelain entries | VERIFIED FROM CODE |
| OS | Ubuntu 26.04 LTS | VERIFIED FROM CODE (environment inspection) |
| Kernel | Linux `7.0.0-29-generic` | VERIFIED FROM CODE |
| Architecture | `x86_64` | VERIFIED FROM CODE |
| C compiler | GCC/cc `15.2.0` | VERIFIED FROM CODE |
| Primary PROFILE B reference | GNU nm (GNU Binutils for Ubuntu) `2.46` | VERIFIED FROM CODE (tool inspection) |
| Supporting inspectors | GNU readelf/objdump `2.46`; file `5.46` | VERIFIED FROM CODE |
| Build command | `make` | VERIFIED FROM CODE (`Makefile:1-38`) |
| Debug build switch | `make DEBUG=1` | VERIFIED FROM CODE (`Makefile:6-13`) |
| Run command | `./woody_woodpacker <elf_file> [hex_key]` | VERIFIED FROM CODE (`include/woody.h:20-23`, `src/arg.c:55-75`) |
| Output | fixed path `./woody`, mode request `0755` | VERIFIED FROM CODE (`src/write_output.c:101-140`) |

README contains only the title `# packer`; therefore build and invocation were
derived from Makefile and source rather than README.

## Source integrity record

The manifest aggregate is the SHA-256 of the sorted per-file SHA-256 lines.

| Manifest | Files | SHA-256 | Evidence |
|---|---:|---|---|
| All Git-tracked files | repository snapshot | `6425a5e89a6e728b415dcefcc34945016638d89a7cb27af5000ffac3e1ec5dd5` | VERIFIED FROM CODE |
| Tracked `*.c`, `*.h`, `*.s`, `*.bin`, and Makefiles | 103 | `6c776a7afc00661df41970e8e8ed94d868cb960d6ed396a616550a077fd6ebb5` | VERIFIED FROM CODE |

Core implementation hashes:

| File | SHA-256 |
|---|---|
| `Makefile` | `89fc043bd68d7c0438b62c8f58b2c285f9fc9e87fd131c3be04caf254726b66e` |
| `include/woody.h` | `3844bafebbc26efc1edc5320eb0c958dc8812bd3d4d04b6c13fc10dd7acc9280` |
| `src/arg.c` | `971e8204095464455ef679877c73e87a1efb96d0a5de47910d26327504e57144` |
| `src/check_meta.c` | `c42fefaeefb6e88f921589490c1f9551bfd8215a016756466c6eebe22ee7f5cc` |
| `src/elf_parser.c` | `07366c066dad69bbe5ae01bf64d3663529f2a3d74ef99c034e9dfc4fbca614bc` |
| `src/encryption_xor.c` | `6afa7145dbcc8abcdc32929a0c3be1aa4b77679770ec1877bfb5851bdb87b032` |
| `src/enter_data.c` | `e43fd0ac5caaae9e1df7e00fd3b009b9b6fcdd353efd978ba1fd53af2a91efe6` |
| `src/error.c` | `ba80e070ca20b05e92df07ba47f3d4e0d36af17cccec1671ff40c34b072e0b3b` |
| `src/format_router.c` | `72d7f31c639e787aff14bf8daf1d58241917916f0b83d7089351bb9fc7753b2a` |
| `src/io_unit.c` | `4425858db61e4e8ec30b311d5898974f60860033234baee92d66a28148962554` |
| `src/shellcode_32.bin` | `c6220848f6761dfca642b586836f7c6c6f519e39447a34d7a65c6a8cc639eb20` |
| `src/shellcode_64.bin` | `5fa59d96813d41369b187d7a8d5a68e87e91b0c9548eb96b1cfce1adc8f66c60` |
| `src/shellcode_data.c` | `0887d255c88b17466d926cec416bde4f8259365f71f8de4dd0ea2accfcb1e540` |
| `src/stub.s` | `87403c89a5c8ff9b514be60845104894b13746c730ae0ba7fbcf85b39f3af6d5` |
| `src/woody-packer.c` | `a10343d51f9c20c665634318df8a3a371cddcc254935df587bbafe22fe65e48e` |
| `src/write_output.c` | `b3a116d8b851d3489fa1ae642567bc7d9696430b83c470d34110cf4c9b688d02` |

At CHECKPOINT A, the target was not built or executed. Existing tracked fixture
`hello` was inspected read-only with `file`, `readelf`, and `nm` to confirm that a
32-bit PIE fixture and GNU nm are available. This is not classified as a target
runtime test.

## Post-approval isolated build and execution

After CHECKPOINT A approval, commit `f69b1dac0d08d0442a126121867b4592cc88fa66`
was exported to a fresh `/tmp/packer-audit-build.*` directory and built there with
`make`. The repository build tree was not used.

| Field | Value | Evidence |
|---|---|---|
| Isolated target | `portfolio_audit/bin/woody_woodpacker` | VERIFIED FROM RUNTIME TEST |
| Target SHA-256 | `cba63f4bed7a296c38a5f59c46419e330d6769564ba5dd685ffad5cf4ebda5ad` | VERIFIED FROM RUNTIME TEST |
| Build result | exit 0; empty stderr | VERIFIED FROM RUNTIME TEST (`raw/build/`) |
| Fixture artifacts | 15 | VERIFIED FROM RUNTIME TEST (`test_results.json`) |
| Target cases | 23 | VERIFIED FROM RUNTIME TEST (`test_results.json`) |

The target was invoked only by the approved adapted PROFILE B harness. Generated
packed files were inspected with GNU nm/readelf/file but were not executed as
programs, so behavioral preservation remains `UNKNOWN`.

## Approved PROFILE E execution note

The later PROFILE E extension executed paired original/packed programs. The default
sandbox blocks 32-bit execution with `SIGSYS`; after explicit approval the entire
suite was run outside that sandbox so ELF32 results would not be misclassified.
All commands remained local and used only generated fixtures or the tracked `hello`.

| Field | Value | Evidence |
|---|---|---|
| PROFILE E fixtures | 8 | VERIFIED FROM RUNTIME TEST |
| PROFILE E cases | 22 | VERIFIED FROM RUNTIME TEST |
| Result | PASS 21, CRASH 1 | VERIFIED FROM RUNTIME TEST |
| Harness validation | 5/5 injected faults detected | VERIFIED FROM RUNTIME TEST |
