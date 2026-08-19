#!/usr/bin/env python3
"""Inject synthetic comparison errors and prove that the harness rejects them."""

from __future__ import annotations

import json
from pathlib import Path

from run_baseline import AUDIT, compare_record_lists, evaluate_rejection, parse_nm_posix


def main() -> int:
	source = AUDIT / "raw" / "B03" / "nm_input.stdout"
	records = parse_nm_posix(source.read_text(encoding="utf-8"))
	if len(records) < 3:
		raise SystemExit("B03 did not provide enough records for harness validation")
	checks = []

	missing = records[:-1]
	checks.append({
		"name": "missing symbol record",
		"detected": not compare_record_lists(records, missing, ordered=False),
	})

	wrong_type = list(records)
	name, symbol_type, value, size = wrong_type[0]
	wrong_type[0] = (name, "?" if symbol_type != "?" else "T", value, size)
	checks.append({
		"name": "wrong symbol type",
		"detected": not compare_record_lists(records, wrong_type, ordered=False),
	})

	wrong_value = list(records)
	value_index = next(index for index, record in enumerate(records) if record[2])
	name, symbol_type, value, size = wrong_value[value_index]
	wrong_value[value_index] = (name, symbol_type, "deadbeef", size)
	checks.append({
		"name": "wrong symbol value",
		"detected": not compare_record_lists(records, wrong_value, ordered=False),
	})

	reversed_records = list(reversed(records))
	checks.append({
		"name": "wrong symbol ordering",
		"detected": not compare_record_lists(records, reversed_records, ordered=True),
	})

	false_success = {
		"returncode": 0,
		"timed_out": False,
		"stderr": "",
	}
	rejection_accepted, _ = evaluate_rejection(false_success, True, "Usage:")
	checks.append({
		"name": "unexpected success/output in rejection case",
		"detected": not rejection_accepted,
	})

	passed = sum(1 for check in checks if check["detected"])
	payload = {"checks": len(checks), "detected": passed, "results": checks}
	output_json = AUDIT / "raw" / "harness_validation.json"
	output_json.write_text(json.dumps(payload, indent=2), encoding="utf-8")
	output_md = AUDIT / "harness_validation.md"
	lines = [
		"# Harness validation",
		"",
		f"Synthetic errors detected: {passed}/{len(checks)}.",
		"",
		"| Injection | Detected? |",
		"|---|---|",
	]
	for check in checks:
		lines.append(f"| {check['name']} | {'YES' if check['detected'] else 'NO'} |")
	output_md.write_text("\n".join(lines) + "\n", encoding="utf-8")
	print(json.dumps(payload, sort_keys=True))
	return 0 if passed == len(checks) else 1


if __name__ == "__main__":
	raise SystemExit(main())
