# PROFILE E baseline results

Fixtures: 8; cases: 22; PASS=21, PARTIAL=0, FAIL=0, CRASH=1, SETUP_FAILURE=0

| ID | Classification | Kind | Case | Evidence summary |
|---|---|---|---|---|
| E01 | PASS | behavior | ELF64 ET_EXEC default behavior | stdout/stderr/exit status matched after one marker normalization |
| E02 | PASS | behavior | ELF64 PIE default behavior | stdout/stderr/exit status matched after one marker normalization |
| E03 | PASS | behavior | ELF64 static syscall behavior | stdout/stderr/exit status matched after one marker normalization |
| E04 | PASS | behavior | ELF32 static syscall behavior | stdout/stderr/exit status matched after one marker normalization |
| E05 | PASS | behavior | ELF32 PIE tracked hello behavior | stdout/stderr/exit status matched after one marker normalization |
| E06 | PASS | behavior | stripped ELF64 behavior | stdout/stderr/exit status matched after one marker normalization |
| E07 | PASS | behavior | argument preservation | stdout/stderr/exit status matched after one marker normalization |
| E08 | PASS | behavior | stdout preservation | stdout/stderr/exit status matched after one marker normalization |
| E09 | PASS | behavior | stderr preservation | stdout/stderr/exit status matched after one marker normalization |
| E10 | PASS | behavior | exit status preservation | stdout/stderr/exit status matched after one marker normalization |
| E11 | PASS | behavior | binary stdin preservation | stdout/stderr/exit status matched after one marker normalization |
| E12 | PASS | behavior | environment-derived behavior | stdout/stderr/exit status matched after one marker normalization |
| E13 | PASS | behavior | all-zero explicit key behavior | stdout/stderr/exit status matched after one marker normalization |
| E14 | PASS | behavior | all-one explicit key behavior | stdout/stderr/exit status matched after one marker normalization |
| E15 | CRASH | behavior | shared object loaded through dlopen | packed program terminated by signal 4 |
| E16 | PASS | structural | ELF64 header and PHDR delta | ELF header/PHDR delta matched |
| E17 | PASS | structural | ELF32 header and PHDR delta | ELF header/PHDR delta matched |
| E18 | PASS | structural | XOR ranges and byte-delta allowlist | encrypted ranges and global byte-delta allowlist matched |
| E19 | PASS | structural | payload shellcode and metadata | payload shellcode/marker/metadata matched |
| E20 | PASS | determinism | same input and key produce identical output | output hashes matched |
| E21 | PASS | structural | original input integrity | input hash unchanged |
| E22 | PASS | structural | output executable mode and ELF identity | output executable mode 755 and ELF identity preserved |

Raw evidence is stored under `raw/profile_e/<case-id>/`.
