# Baseline test results

Adapted PROFILE B; GNU nm observes symbol metadata before and after packing.

Target SHA-256: `cba63f4bed7a296c38a5f59c46419e330d6769564ba5dd685ffad5cf4ebda5ad`

Fixtures: 15; cases: 23; PASS=23, PARTIAL=0, FAIL=0, CRASH=0, SETUP_FAILURE=0

| ID | Classification | Case | Evidence summary |
|---|---|---|---|
| B01 | PASS | no argument | graceful expected rejection |
| B02 | PASS | malformed explicit key | graceful expected rejection |
| B03 | PASS | ELF64 non-PIE full symbol set | paired GNU nm observation matched |
| B04 | PASS | ELF64 PIE full symbol set | paired GNU nm observation matched |
| B05 | PASS | ELF32 non-PIE symbols | paired GNU nm observation matched |
| B06 | PASS | tracked ELF32 PIE hello | paired GNU nm observation matched |
| B07 | PASS | stripped ELF64 | paired GNU nm observation matched |
| B08 | PASS | local/global scoped symbols | paired GNU nm observation matched |
| B09 | PASS | weak defined and undefined symbols | paired GNU nm observation matched |
| B10 | PASS | undefined imported symbols | paired GNU nm observation matched |
| B11 | PASS | text/rodata/data/BSS/absolute types | paired GNU nm observation matched |
| B12 | PASS | numeric symbol ordering | paired GNU nm observation matched |
| B13 | PASS | global symbol inclusion | paired GNU nm observation matched |
| B14 | PASS | shared object | paired GNU nm observation matched |
| B15 | PASS | relocatable object rejection | graceful expected rejection |
| B16 | PASS | static archive rejection | graceful expected rejection |
| B17 | PASS | non-ELF rejection | graceful expected rejection |
| B18 | PASS | truncated ELF rejection | graceful expected rejection |
| B19a | PASS | invalid ELF class rejection | graceful expected rejection |
| B19b | PASS | big-endian rejection | graceful expected rejection |
| B20 | PASS | unsupported machine rejection | graceful expected rejection |
| B21 | PASS | out-of-range PHDR rejection | graceful expected rejection |
| B22 | PASS | no eligible stub segment | graceful expected rejection |

All target/reference stdout, stderr, exit status, hashes, and structural inspection
are retained under `raw/<case-id>/`.
