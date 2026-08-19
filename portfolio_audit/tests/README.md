# Deterministic test inputs

`fixtures/` contains the human-authored sources. `generated/` and the per-case
working directories are produced by `tools/run_baseline.py`.

The harness always uses an explicit fixed XOR key and isolates every invocation
because the target writes to a fixed filename, `woody`.
