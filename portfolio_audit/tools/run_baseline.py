#!/usr/bin/env python3
"""Build deterministic fixtures and run the adapted PROFILE B baseline suite."""

from __future__ import annotations

import collections
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import time
from pathlib import Path
from typing import Any, Iterable


AUDIT = Path(__file__).resolve().parents[1]
REPO = AUDIT.parent
TARGET = AUDIT / "bin" / "woody_woodpacker"
FIXTURE_SRC = AUDIT / "tests" / "fixtures"
GENERATED = AUDIT / "tests" / "generated"
RAW = AUDIT / "raw"
SETUP_RAW = RAW / "setup"
RESULTS_JSON = AUDIT / "test_results.json"
RESULTS_MD = AUDIT / "test_results.md"
FIXED_KEY = "0123456789abcdef"
ENV = {**os.environ, "LC_ALL": "C", "LANG": "C"}


def text_value(value: Any) -> str:
	if value is None:
		return ""
	if isinstance(value, bytes):
		return value.decode("utf-8", errors="replace")
	return str(value)


def run(command: list[str], cwd: Path | None = None, timeout_s: int = 10) -> dict[str, Any]:
	start = time.monotonic()
	try:
		completed = subprocess.run(
			command,
			cwd=cwd,
			env=ENV,
			capture_output=True,
			text=True,
			errors="replace",
			timeout=timeout_s,
			check=False,
		)
		return {
			"command": command,
			"returncode": completed.returncode,
			"stdout": completed.stdout,
			"stderr": completed.stderr,
			"timed_out": False,
			"duration_ms": round((time.monotonic() - start) * 1000, 3),
		}
	except subprocess.TimeoutExpired as exc:
		return {
			"command": command,
			"returncode": None,
			"stdout": text_value(exc.stdout),
			"stderr": text_value(exc.stderr),
			"timed_out": True,
			"duration_ms": round((time.monotonic() - start) * 1000, 3),
		}


def write_run(directory: Path, stem: str, result: dict[str, Any]) -> None:
	directory.mkdir(parents=True, exist_ok=True)
	(directory / f"{stem}.command.txt").write_text(
		" ".join(result["command"]) + "\n", encoding="utf-8"
	)
	(directory / f"{stem}.stdout").write_text(result["stdout"], encoding="utf-8")
	(directory / f"{stem}.stderr").write_text(result["stderr"], encoding="utf-8")
	(directory / f"{stem}.status.json").write_text(
		json.dumps(
			{
				"returncode": result["returncode"],
				"timed_out": result["timed_out"],
				"duration_ms": result["duration_ms"],
			},
			indent=2,
			),
		encoding="utf-8",
	)


def sha256(path: Path) -> str:
	digest = hashlib.sha256()
	with path.open("rb") as stream:
		for block in iter(lambda: stream.read(1024 * 1024), b""):
			digest.update(block)
	return digest.hexdigest()


def setup_command(name: str, command: list[str]) -> dict[str, Any]:
	result = run(command, timeout_s=30)
	write_run(SETUP_RAW, name, result)
	return result


def mutate_elf64(source: Path, destination: Path, mutation: str) -> None:
	data = bytearray(source.read_bytes())
	if mutation == "class":
		data[4] = 0
	elif mutation == "endian":
		data[5] = 2
	elif mutation == "machine":
		struct.pack_into("<H", data, 18, 183)  # EM_AARCH64
	elif mutation == "phdr_oob":
		struct.pack_into("<Q", data, 32, len(data) - 1)
	elif mutation == "no_stub":
		phoff = struct.unpack_from("<Q", data, 32)[0]
		phentsize = struct.unpack_from("<H", data, 54)[0]
		phnum = struct.unpack_from("<H", data, 56)[0]
		changed = 0
		for index in range(phnum):
			offset = phoff + index * phentsize
			p_type, p_flags = struct.unpack_from("<II", data, offset)
			p_offset = struct.unpack_from("<Q", data, offset + 8)[0]
			if p_type == 1 and p_offset != 0:  # PT_LOAD
				struct.pack_into("<I", data, offset + 4, 2)  # PF_W only
				changed += 1
		if changed == 0:
			raise RuntimeError("no nonzero-offset PT_LOAD was available to mutate")
	else:
		raise ValueError(mutation)
	destination.write_bytes(data)


def prepare_directories() -> None:
	if GENERATED.exists():
		shutil.rmtree(GENERATED)
	GENERATED.mkdir(parents=True)
	if SETUP_RAW.exists():
		shutil.rmtree(SETUP_RAW)
	SETUP_RAW.mkdir(parents=True)
	for path in RAW.glob("B[0-9][0-9]*"):
		if path.is_dir():
			shutil.rmtree(path)


def build_fixtures() -> tuple[dict[str, Path], dict[str, str]]:
	prepare_directories()
	fixtures = {
		"elf64_exec": GENERATED / "elf64_exec",
		"elf64_pie": GENERATED / "elf64_pie",
		"elf32_exec": GENERATED / "elf32_exec",
		"elf32_pie": GENERATED / "elf32_pie_hello",
		"stripped64": GENERATED / "stripped64",
		"shared64": GENERATED / "libshared_fixture.so",
		"rel64": GENERATED / "symbol_fixture.o",
		"archive": GENERATED / "libfixture.a",
		"not_elf": GENERATED / "not_elf.txt",
		"truncated": GENERATED / "truncated_elf",
		"bad_class": GENERATED / "bad_class",
		"bad_endian": GENERATED / "bad_endian",
		"bad_machine": GENERATED / "bad_machine",
		"bad_phdr": GENERATED / "bad_phdr",
		"no_stub": GENERATED / "no_stub",
	}
	commands = {
		"elf64_exec": [
			"gcc", "-O0", "-fno-inline", "-fno-pie", "-no-pie",
			"-Wl,--build-id=none", "-o", str(fixtures["elf64_exec"]),
			str(FIXTURE_SRC / "symbol_fixture.c"),
		],
		"elf64_pie": [
			"gcc", "-O0", "-fno-inline", "-fPIE", "-pie",
			"-Wl,--build-id=none", "-o", str(fixtures["elf64_pie"]),
			str(FIXTURE_SRC / "symbol_fixture.c"),
		],
		"elf32_exec": [
			"gcc", "-m32", "-nostdlib", "-fno-pie", "-no-pie",
			"-Wl,--build-id=none", "-o", str(fixtures["elf32_exec"]),
			str(FIXTURE_SRC / "symbol_fixture32.s"),
		],
		"shared64": [
			"gcc", "-O0", "-fno-inline", "-shared", "-fPIC",
			"-Wl,--build-id=none", "-o", str(fixtures["shared64"]),
			str(FIXTURE_SRC / "shared_fixture.c"),
		],
		"rel64": [
			"gcc", "-O0", "-fno-inline", "-c", "-o", str(fixtures["rel64"]),
			str(FIXTURE_SRC / "symbol_fixture.c"),
		],
	}
	failures: dict[str, str] = {}
	for name, command in commands.items():
		result = setup_command(f"build_{name}", command)
		if result["returncode"] != 0 or result["timed_out"]:
			failures[name] = "fixture compile failed"

	if "elf64_exec" not in failures:
		shutil.copy2(fixtures["elf64_exec"], fixtures["stripped64"])
		result = setup_command("strip_elf64", ["strip", "--strip-all", str(fixtures["stripped64"])])
		if result["returncode"] != 0:
			failures["stripped64"] = "strip failed"
	else:
		failures["stripped64"] = "base fixture unavailable"

	if "rel64" not in failures:
		result = setup_command("build_archive", ["ar", "rcs", str(fixtures["archive"]), str(fixtures["rel64"])])
		if result["returncode"] != 0:
			failures["archive"] = "archive creation failed"
	else:
		failures["archive"] = "object fixture unavailable"

	shutil.copy2(REPO / "hello", fixtures["elf32_pie"])
	shutil.copy2(FIXTURE_SRC / "not_elf.txt", fixtures["not_elf"])
	if "elf64_pie" not in failures:
		base = fixtures["elf64_pie"]
		fixtures["truncated"].write_bytes(base.read_bytes()[:8])
		for key, mutation in (
			("bad_class", "class"),
			("bad_endian", "endian"),
			("bad_machine", "machine"),
			("bad_phdr", "phdr_oob"),
			("no_stub", "no_stub"),
		):
			try:
				mutate_elf64(base, fixtures[key], mutation)
			except Exception as exc:  # setup failure is reported separately
				failures[key] = f"mutation failed: {exc}"
	else:
		for key in ("truncated", "bad_class", "bad_endian", "bad_machine", "bad_phdr", "no_stub"):
			failures[key] = "base fixture unavailable"

	for name, path in fixtures.items():
		if name not in failures and not path.exists():
			failures[name] = "fixture file was not created"
	(SETUP_RAW / "setup_summary.json").write_text(
		json.dumps({"fixtures": {key: str(value) for key, value in fixtures.items()}, "failures": failures}, indent=2),
		encoding="utf-8",
	)
	return fixtures, failures


def parse_nm_posix(output: str) -> list[tuple[str, str, str, str]]:
	records: list[tuple[str, str, str, str]] = []
	for line in output.splitlines():
		parts = line.split()
		if len(parts) < 2:
			continue
		parts.extend([""] * (4 - len(parts)))
		records.append((parts[0], parts[1], parts[2], parts[3]))
	return records


def filter_records(
	records: Iterable[tuple[str, str, str, str]], prefixes: list[str] | None
) -> list[tuple[str, str, str, str]]:
	if not prefixes:
		return list(records)
	return [record for record in records if any(record[0].startswith(prefix) for prefix in prefixes)]


def compare_record_lists(
	left: list[tuple[str, str, str, str]],
	right: list[tuple[str, str, str, str]],
	ordered: bool,
) -> bool:
	if ordered:
		return left == right
	return collections.Counter(left) == collections.Counter(right)


def normalize_diagnostic(text: str) -> str:
	lines = []
	for line in text.replace("\r\n", "\n").splitlines():
		if line.startswith("nm: ") and line.count(": ") >= 2:
			prefix, _path, message = line.split(": ", 2)
			line = f"{prefix}: <file>: {message}"
		else:
			line = re.sub(r"^.*?:", "<file>:", line, count=1)
		lines.append(line.rstrip(" \t"))
	return "\n".join(lines)


def inspect_file(case_dir: Path, label: str, path: Path) -> None:
	(case_dir / f"{label}.sha256").write_text(f"{sha256(path)}  {path.name}\n", encoding="utf-8")
	for tool, command in (
		("file", ["file", str(path)]),
		("readelf_header", ["readelf", "-h", str(path)]),
		("readelf_program_headers", ["readelf", "-lW", str(path)]),
	):
		result = run(command)
		write_run(case_dir, f"{label}.{tool}", result)


def evaluate_rejection(result: dict[str, Any], output_exists: bool, needle: str) -> tuple[bool, str]:
	if result["timed_out"]:
		return False, "target timed out"
	if result["returncode"] is not None and result["returncode"] < 0:
		return False, f"target terminated by signal {-result['returncode']}"
	if result["returncode"] == 0:
		return False, "target unexpectedly returned success"
	if output_exists:
		return False, "target unexpectedly created woody"
	if needle not in result["stderr"]:
		return False, f"diagnostic did not contain {needle!r}"
	return True, "graceful expected rejection"


def compare_nm(
	case_dir: Path,
	input_path: Path,
	output_path: Path,
	nm_args: list[str],
	ordered: bool,
	prefixes: list[str] | None,
) -> tuple[bool, str, dict[str, Any]]:
	input_result = run(["nm", *nm_args, str(input_path)])
	output_result = run(["nm", *nm_args, str(output_path)])
	write_run(case_dir, "nm_input", input_result)
	write_run(case_dir, "nm_output", output_result)
	left = filter_records(parse_nm_posix(input_result["stdout"]), prefixes)
	right = filter_records(parse_nm_posix(output_result["stdout"]), prefixes)
	details = {
		"nm_args": nm_args,
		"ordered": ordered,
		"prefixes": prefixes,
		"input_returncode": input_result["returncode"],
		"output_returncode": output_result["returncode"],
		"input_record_count": len(left),
		"output_record_count": len(right),
		"input_records": left,
		"output_records": right,
	}
	if prefixes:
		missing = [prefix for prefix in prefixes if not any(record[0].startswith(prefix) for record in left)]
		if missing:
			return False, f"SETUP: input lacks expected symbols {missing}", details
	if input_result["returncode"] != output_result["returncode"]:
		return False, "GNU nm exit status changed", details
	if normalize_diagnostic(input_result["stderr"]) != normalize_diagnostic(output_result["stderr"]):
		return False, "GNU nm diagnostic semantics changed", details
	if not compare_record_lists(left, right, ordered):
		return False, "GNU nm symbol records changed", details
	return True, "paired GNU nm observation matched", details


def case_specs(fixtures: dict[str, Path]) -> list[dict[str, Any]]:
	return [
		{"id": "B01", "title": "no argument", "expect": "reject", "needle": "Usage:"},
		{"id": "B02", "title": "malformed explicit key", "fixture": "elf64_exec", "expect": "reject", "needle": "Invalid key format", "key": "xyz"},
		{"id": "B03", "title": "ELF64 non-PIE full symbol set", "fixture": "elf64_exec", "expect": "pack", "nm": ["--format=posix"]},
		{"id": "B04", "title": "ELF64 PIE full symbol set", "fixture": "elf64_pie", "expect": "pack", "nm": ["--format=posix"]},
		{"id": "B05", "title": "ELF32 non-PIE symbols", "fixture": "elf32_exec", "expect": "pack", "nm": ["--format=posix"]},
		{"id": "B06", "title": "tracked ELF32 PIE hello", "fixture": "elf32_pie", "expect": "pack", "nm": ["--format=posix"]},
		{"id": "B07", "title": "stripped ELF64", "fixture": "stripped64", "expect": "pack", "nm": ["--format=posix"]},
		{"id": "B08", "title": "local/global scoped symbols", "fixture": "elf64_exec", "expect": "pack", "nm": ["--format=posix"], "prefixes": ["scope_local_value", "scope_global_value"]},
		{"id": "B09", "title": "weak defined and undefined symbols", "fixture": "shared64", "expect": "pack", "nm": ["--format=posix"], "prefixes": ["shared_weak_function", "shared_weak_undefined_function"]},
		{"id": "B10", "title": "undefined imported symbols", "fixture": "elf64_exec", "expect": "pack", "nm": ["--format=posix"], "prefixes": ["printf@"]},
		{"id": "B11", "title": "text/rodata/data/BSS/absolute types", "fixture": "elf64_exec", "expect": "pack", "nm": ["--format=posix"], "prefixes": ["global_text_symbol", "global_rodata_symbol", "global_data_symbol", "global_bss_symbol", "absolute_symbol"]},
		{"id": "B12", "title": "numeric symbol ordering", "fixture": "elf64_exec", "expect": "pack", "nm": ["-n", "--format=posix"], "ordered": True},
		{"id": "B13", "title": "global symbol inclusion", "fixture": "elf64_exec", "expect": "pack", "nm": ["-g", "--format=posix"]},
		{"id": "B14", "title": "shared object", "fixture": "shared64", "expect": "pack_or_partial", "nm": ["--format=posix"]},
		{"id": "B15", "title": "relocatable object rejection", "fixture": "rel64", "expect": "reject", "needle": "Invalid ELF type"},
		{"id": "B16", "title": "static archive rejection", "fixture": "archive", "expect": "reject", "needle": "Unknown file format"},
		{"id": "B17", "title": "non-ELF rejection", "fixture": "not_elf", "expect": "reject", "needle": "Unknown file format"},
		{"id": "B18", "title": "truncated ELF rejection", "fixture": "truncated", "expect": "reject", "needle": "Invalid program header table"},
		{"id": "B19a", "title": "invalid ELF class rejection", "fixture": "bad_class", "expect": "reject", "needle": "Invalid ELF class"},
		{"id": "B19b", "title": "big-endian rejection", "fixture": "bad_endian", "expect": "reject", "needle": "Only little-endian supported"},
		{"id": "B20", "title": "unsupported machine rejection", "fixture": "bad_machine", "expect": "reject", "needle": "Invalid machine type"},
		{"id": "B21", "title": "out-of-range PHDR rejection", "fixture": "bad_phdr", "expect": "reject", "needle": "Invalid program header table"},
		{"id": "B22", "title": "no eligible stub segment", "fixture": "no_stub", "expect": "reject", "needle": "No space for shellcode injection"},
	]


def run_case(spec: dict[str, Any], fixtures: dict[str, Path], setup_failures: dict[str, str]) -> dict[str, Any]:
	case_id = spec["id"]
	case_dir = RAW / case_id
	work_dir = case_dir / "work"
	work_dir.mkdir(parents=True)
	fixture_name = spec.get("fixture")
	input_path = fixtures.get(fixture_name) if fixture_name else None
	base_result = {"id": case_id, "title": spec["title"], "expectation": spec["expect"]}
	if fixture_name in setup_failures:
		return {**base_result, "classification": "SETUP_FAILURE", "reason": setup_failures[fixture_name]}
	if input_path is not None:
		inspect_file(case_dir, "input", input_path)
	command = [str(TARGET)]
	if input_path is not None:
		command.append(str(input_path))
		command.append(spec.get("key", FIXED_KEY))
	target_result = run(command, cwd=work_dir, timeout_s=5)
	write_run(case_dir, "target", target_result)
	output_path = work_dir / "woody"
	output_exists = output_path.is_file()
	base_result.update(
		{
			"target_returncode": target_result["returncode"],
			"target_timed_out": target_result["timed_out"],
			"output_exists": output_exists,
			"target_stdout": target_result["stdout"],
			"target_stderr": target_result["stderr"],
		}
	)
	if output_exists:
		inspect_file(case_dir, "output", output_path)
	if target_result["timed_out"] or (
		target_result["returncode"] is not None and target_result["returncode"] < 0
	):
		reason = "timeout" if target_result["timed_out"] else f"signal {-target_result['returncode']}"
		return {**base_result, "classification": "CRASH", "reason": reason}
	if spec["expect"] == "reject":
		passed, reason = evaluate_rejection(target_result, output_exists, spec["needle"])
		return {**base_result, "classification": "PASS" if passed else "FAIL", "reason": reason}
	if target_result["returncode"] != 0 or not output_exists:
		classification = "PARTIAL" if spec["expect"] == "pack_or_partial" else "FAIL"
		return {
			**base_result,
			"classification": classification,
			"reason": "admitted file did not produce a comparable packed output",
		}
	passed, reason, details = compare_nm(
		case_dir,
		input_path,
		output_path,
		spec.get("nm", ["--format=posix"]),
		spec.get("ordered", False),
		spec.get("prefixes"),
	)
	classification = "PASS" if passed else ("SETUP_FAILURE" if reason.startswith("SETUP:") else "FAIL")
	return {**base_result, "classification": classification, "reason": reason, "comparison": details}


def write_summary(
	results: list[dict[str, Any]],
	setup_failures: dict[str, str],
	fixtures: dict[str, Path],
) -> None:
	observed_counts = collections.Counter(result["classification"] for result in results)
	counts = {
		category: observed_counts.get(category, 0)
		for category in ("PASS", "PARTIAL", "FAIL", "CRASH", "SETUP_FAILURE")
	}
	payload = {
		"profile": "PROFILE B adapted to packer as symbol-preservation observation",
		"target": str(TARGET),
		"target_sha256": sha256(TARGET),
		"reference": "GNU nm 2.46",
		"fixed_key": FIXED_KEY,
		"fixture_count": len(fixtures),
		"case_count": len(results),
		"counts": counts,
		"setup_failures": setup_failures,
		"results": results,
	}
	RESULTS_JSON.write_text(json.dumps(payload, indent=2), encoding="utf-8")
	lines = [
		"# Baseline test results",
		"",
		"Adapted PROFILE B; GNU nm observes symbol metadata before and after packing.",
		"",
		f"Target SHA-256: `{payload['target_sha256']}`",
		"",
		f"Fixtures: {len(fixtures)}; cases: {len(results)}; "
		+ ", ".join(f"{key}={value}" for key, value in counts.items()),
		"",
		"| ID | Classification | Case | Evidence summary |",
		"|---|---|---|---|",
	]
	for result in results:
		reason = result["reason"].replace("|", "\\|").replace("\n", " ")
		lines.append(f"| {result['id']} | {result['classification']} | {result['title']} | {reason} |")
	lines.extend(
		[
			"",
			"All target/reference stdout, stderr, exit status, hashes, and structural inspection",
			"are retained under `raw/<case-id>/`.",
		]
	)
	RESULTS_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
	if not TARGET.is_file():
		raise SystemExit(f"missing isolated target binary: {TARGET}")
	fixtures, setup_failures = build_fixtures()
	results = [run_case(spec, fixtures, setup_failures) for spec in case_specs(fixtures)]
	write_summary(results, setup_failures, fixtures)
	print(json.dumps(collections.Counter(result["classification"] for result in results), sort_keys=True))
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
