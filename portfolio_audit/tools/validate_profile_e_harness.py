#!/usr/bin/env python3
"""Validate PROFILE E behavior and structure comparators with synthetic faults."""

from __future__ import annotations

import json
import shutil
from pathlib import Path

from run_profile_e import AUDIT, MARKER, analyze_structure, compare_behavior


PROFILE_RAW = AUDIT / "raw" / "profile_e"


def read_program_result(case_id: str, stem: str) -> dict:
	directory = PROFILE_RAW / case_id
	status = json.loads((directory / f"{stem}.status.json").read_text(encoding="utf-8"))
	return {
		"returncode": status["returncode"],
		"timed_out": status["timed_out"],
		"stdout": (directory / f"{stem}.stdout.bin").read_bytes(),
		"stderr": (directory / f"{stem}.stderr.bin").read_bytes(),
	}


def main() -> int:
	validation = PROFILE_RAW / "harness_validation"
	if validation.exists():
		shutil.rmtree(validation)
	validation.mkdir(parents=True)
	original = read_program_result("E01", "original")
	packed = read_program_result("E01", "packed")
	control, _reason, _details = compare_behavior(original, packed)
	if control != "PASS":
		raise SystemExit("E01 control comparison did not pass")
	checks = []

	wrong_exit = dict(packed)
	wrong_exit["returncode"] = 9
	classification, _reason, _details = compare_behavior(original, wrong_exit)
	checks.append({"name": "wrong exit status", "detected": classification != "PASS"})

	extra_stdout = dict(packed)
	extra_stdout["stdout"] = packed["stdout"] + b"unexpected-output\n"
	classification, _reason, _details = compare_behavior(original, extra_stdout)
	checks.append({"name": "extra stdout after marker", "detected": classification != "PASS"})

	missing_marker = dict(packed)
	if not packed["stdout"].startswith(MARKER):
		raise SystemExit("E01 packed control lacked marker")
	missing_marker["stdout"] = packed["stdout"][len(MARKER):]
	classification, _reason, _details = compare_behavior(original, missing_marker)
	checks.append({"name": "missing marker", "detected": classification != "PASS"})

	e16 = PROFILE_RAW / "E16"
	e16_input = AUDIT / "tests" / "profile_e_generated" / "behavior_exec64"
	e16_output = e16 / "pack" / "woody"
	corrupt_header = validation / "corrupt_header"
	data = bytearray(e16_output.read_bytes())
	data[7] ^= 1  # EI_OSABI: outside every permitted delta range
	corrupt_header.write_bytes(data)
	analysis = analyze_structure(e16_input, corrupt_header, "0123456789abcdef")
	checks.append({"name": "unexpected ELF-header byte delta", "detected": not analysis.get("ok", False)})

	e19 = PROFILE_RAW / "E19"
	e19_input = AUDIT / "tests" / "profile_e_generated" / "behavior_static64"
	e19_output = e19 / "pack" / "woody"
	structure = json.loads((e19 / "structure.json").read_text(encoding="utf-8"))
	corrupt_metadata = validation / "corrupt_metadata"
	data = bytearray(e19_output.read_bytes())
	key_offset = structure["payload_offset"] + structure["shellcode_size"] + 16 + 16
	data[key_offset] ^= 1
	corrupt_metadata.write_bytes(data)
	analysis = analyze_structure(e19_input, corrupt_metadata, "0123456789abcdef")
	checks.append({"name": "corrupt payload key metadata", "detected": not analysis.get("checks", {}).get("payload_metadata_correct", True)})

	detected = sum(1 for check in checks if check["detected"])
	payload = {"checks": len(checks), "detected": detected, "results": checks}
	(validation / "results.json").write_text(json.dumps(payload, indent=2), encoding="utf-8")
	output = AUDIT / "profile_e_harness_validation.md"
	lines = [
		"# PROFILE E harness validation",
		"",
		f"Synthetic errors detected: {detected}/{len(checks)}.",
		"",
		"| Injection | Detected? |",
		"|---|---|",
	]
	for check in checks:
		lines.append(f"| {check['name']} | {'YES' if check['detected'] else 'NO'} |")
	output.write_text("\n".join(lines) + "\n", encoding="utf-8")
	print(json.dumps(payload, sort_keys=True))
	return 0 if detected == len(checks) else 1


if __name__ == "__main__":
	raise SystemExit(main())
