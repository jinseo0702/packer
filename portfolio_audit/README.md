# packer audit — CHECKPOINT B + approved PROFILE E extension

Status: CHECKPOINT B reached; waiting for user direction.

No existing project source was modified. After CHECKPOINT A approval, an isolated
target build, deterministic fixtures, the adapted PROFILE B suite, raw evidence,
and harness validation were added under this directory.

Evidence labels used throughout:

- `VERIFIED FROM CODE`: established by reading the repository at the recorded commit.
- `VERIFIED FROM RUNTIME TEST`: established by executing the target (none at this checkpoint).
- `USER-PROVIDED RATIONALE`: explicitly supplied by the user (none at this checkpoint).
- `INFERENCE`: a reasoned conclusion that still needs a runtime test.
- `UNKNOWN`: not established by the evidence collected so far.

Documents:

- [baseline.md](baseline.md): commit, environment, commands, and integrity record.
- [architecture.md](architecture.md): code-derived architecture and design characteristics.
- [feature_inventory.md](feature_inventory.md): implementation status and functional scope.
- [test_plan.md](test_plan.md): PROFILE B oracle, planned cases, and normalization rules.
- [test_results.md](test_results.md) and [test_results.json](test_results.json): baseline results.
- [harness_validation.md](harness_validation.md): five synthetic error-detection checks.
- [failures.md](failures.md): failure analysis and scope/correctness separation.
- [design_rationale.md](design_rationale.md): verified facts, user rationale, and inference.
- [profile_e_test_plan.md](profile_e_test_plan.md): behavioral/structural oracle and normalization.
- [profile_e_test_results.md](profile_e_test_results.md) and
  [profile_e_test_results.json](profile_e_test_results.json): PROFILE E baseline.
- [profile_e_harness_validation.md](profile_e_harness_validation.md): PROFILE E synthetic checks.
- [profile_e_failures.md](profile_e_failures.md): shared-object CRASH root cause.
- [improvement.md](improvement.md): ranked candidates and recommended first correction.

## CHECKPOINT B result

- 15 generated fixture artifacts and 23 target cases.
- PASS 23, PARTIAL 0, FAIL 0, CRASH 0, SETUP_FAILURE 0.
- 12 admitted ELF cases produced `woody` and retained the paired GNU nm
  observations used by their focused comparison.
- 11 CLI/unsupported/malformed/layout cases returned the expected graceful
  rejection without an output file.
- Harness validation detected 5/5 deliberately injected errors: missing symbol,
  wrong type, wrong value, wrong order, and an unexpected success/output.

Important PASS evidence includes 40 symbol records for ELF64 ET_EXEC (B03), 43 for
ELF64 PIE (B04), 10 for ELF32 ET_EXEC (B05), 34 for the tracked ELF32 PIE (B06),
and 25 for a shared object (B14). Counts are per-case GNU nm records, not unique
project capabilities.

## Approved PROFILE E extension result

- 8 fixture artifacts and 22 focused cases.
- PASS 21, PARTIAL 0, FAIL 0, CRASH 1, SETUP_FAILURE 0.
- ELF64 ET_EXEC/PIE/static/stripped, ELF32 static/PIE, argv, stdout, stderr, exit
  status, binary stdin, environment, boundary keys, deterministic output, input
  integrity, and intended structural deltas passed.
- E15: a shared object admitted as `ET_DYN` passed every structural check but its
  packed form raised `SIGILL` when loaded through `dlopen`.
- PROFILE E harness validation detected 5/5 synthetic behavioral/structural faults.

## Profile applicability decision

`VERIFIED FROM CODE` (the supplied audit specification): PROFILE B is defined for
`gnu_nm_project`; the packer-specific profile is PROFILE E.

`INFERENCE`: To obey the explicit request to use PROFILE B on this repository, the
planned semantic comparison treats GNU nm as an observer of the input and output
ELF files. It will compare symbol name, value, type, inclusion/exclusion, and
ordering before and after packing. The target itself does not implement an nm-like
symbol listing interface, so a direct target-output-versus-GNU-nm comparison is not
possible.

Consequently, PROFILE B alone cannot establish the packer's central claim that the
transformed executable preserves program behavior. That limitation is deliberately
left visible rather than silently substituting PROFILE E.
