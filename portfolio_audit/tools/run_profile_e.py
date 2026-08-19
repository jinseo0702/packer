#!/usr/bin/env python3
"""Run the approved PROFILE E behavioral and structural baseline."""

from __future__ import annotations

import collections
import hashlib
import json
import os
import shutil
import stat
import struct
import subprocess
import time
from pathlib import Path
from typing import Any


AUDIT = Path(__file__).resolve().parents[1]
REPO = AUDIT.parent
TARGET = AUDIT / "bin" / "woody_woodpacker"
FIXTURE_SRC = AUDIT / "tests" / "fixtures"
GENERATED = AUDIT / "tests" / "profile_e_generated"
PROFILE_RAW = AUDIT / "raw" / "profile_e"
SETUP_RAW = PROFILE_RAW / "setup"
RESULTS_JSON = AUDIT / "profile_e_test_results.json"
RESULTS_MD = AUDIT / "profile_e_test_results.md"
FIXED_KEY = "0123456789abcdef"
MARKER = b"....WOODY....\n"
ENV = {**os.environ, "LC_ALL": "C", "LANG": "C"}


def sha256(path: Path) -> str:
	digest = hashlib.sha256()
	with path.open("rb") as stream:
		for block in iter(lambda: stream.read(1024 * 1024), b""):
			digest.update(block)
	return digest.hexdigest()


def run_text(command: list[str], cwd: Path | None = None, timeout_s: int = 15) -> dict[str, Any]:
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
			"stdout": (exc.stdout or b"").decode("utf-8", errors="replace") if isinstance(exc.stdout, bytes) else (exc.stdout or ""),
			"stderr": (exc.stderr or b"").decode("utf-8", errors="replace") if isinstance(exc.stderr, bytes) else (exc.stderr or ""),
			"timed_out": True,
			"duration_ms": round((time.monotonic() - start) * 1000, 3),
		}


def write_text_run(directory: Path, stem: str, result: dict[str, Any]) -> None:
	directory.mkdir(parents=True, exist_ok=True)
	(directory / f"{stem}.command.txt").write_text(" ".join(result["command"]) + "\n", encoding="utf-8")
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


def run_program(
	executable: Path,
	arguments: list[str],
	stdin_data: bytes,
	cwd: Path,
	environment: dict[str, str] | None = None,
	timeout_s: int = 5,
) -> dict[str, Any]:
	argv = ["profile_e_program", *arguments]
	program_env = dict(ENV)
	if environment:
		program_env.update(environment)
	start = time.monotonic()
	process = subprocess.Popen(
		argv,
		executable=str(executable),
		cwd=cwd,
		env=program_env,
		stdin=subprocess.PIPE,
		stdout=subprocess.PIPE,
		stderr=subprocess.PIPE,
	)
	try:
		stdout, stderr = process.communicate(stdin_data, timeout=timeout_s)
		return {
			"executable": str(executable),
			"argv": argv,
			"returncode": process.returncode,
			"stdout": stdout,
			"stderr": stderr,
			"timed_out": False,
			"duration_ms": round((time.monotonic() - start) * 1000, 3),
		}
	except subprocess.TimeoutExpired:
		process.kill()
		stdout, stderr = process.communicate()
		return {
			"executable": str(executable),
			"argv": argv,
			"returncode": None,
			"stdout": stdout,
			"stderr": stderr,
			"timed_out": True,
			"duration_ms": round((time.monotonic() - start) * 1000, 3),
		}


def write_program_run(directory: Path, stem: str, result: dict[str, Any]) -> None:
	directory.mkdir(parents=True, exist_ok=True)
	(directory / f"{stem}.command.json").write_text(
		json.dumps({"executable": result["executable"], "argv": result["argv"]}, indent=2),
		encoding="utf-8",
	)
	(directory / f"{stem}.stdout.bin").write_bytes(result["stdout"])
	(directory / f"{stem}.stderr.bin").write_bytes(result["stderr"])
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


def compare_behavior(original: dict[str, Any], packed: dict[str, Any], marker_required: bool = True) -> tuple[str, str, dict[str, Any]]:
	details = {
		"original_returncode": original["returncode"],
		"packed_returncode": packed["returncode"],
		"original_timed_out": original["timed_out"],
		"packed_timed_out": packed["timed_out"],
		"original_stdout_sha256": hashlib.sha256(original["stdout"]).hexdigest(),
		"packed_stdout_sha256": hashlib.sha256(packed["stdout"]).hexdigest(),
		"original_stderr_sha256": hashlib.sha256(original["stderr"]).hexdigest(),
		"packed_stderr_sha256": hashlib.sha256(packed["stderr"]).hexdigest(),
	}
	if original["timed_out"] or (original["returncode"] is not None and original["returncode"] < 0):
		return "SETUP_FAILURE", "original fixture did not complete normally", details
	if packed["timed_out"]:
		return "CRASH", "packed program timed out", details
	if packed["returncode"] is not None and packed["returncode"] < 0:
		return "CRASH", f"packed program terminated by signal {-packed['returncode']}", details
	normalized_stdout = packed["stdout"]
	if marker_required:
		if not normalized_stdout.startswith(MARKER):
			return "FAIL", "packed stdout lacked the required leading marker", details
		normalized_stdout = normalized_stdout[len(MARKER):]
	if original["returncode"] != packed["returncode"]:
		return "FAIL", "exit status changed", details
	if original["stderr"] != packed["stderr"]:
		return "FAIL", "stderr changed", details
	if original["stdout"] != normalized_stdout:
		return "FAIL", "stdout changed after marker normalization", details
	return "PASS", "stdout/stderr/exit status matched after one marker normalization", details


def prepare_directories() -> None:
	if GENERATED.exists():
		shutil.rmtree(GENERATED)
	if PROFILE_RAW.exists():
		shutil.rmtree(PROFILE_RAW)
	GENERATED.mkdir(parents=True)
	SETUP_RAW.mkdir(parents=True)


def setup_command(name: str, command: list[str]) -> dict[str, Any]:
	result = run_text(command, timeout_s=30)
	write_text_run(SETUP_RAW, name, result)
	return result


def build_fixtures() -> tuple[dict[str, Path], dict[str, str]]:
	prepare_directories()
	fixtures = {
		"dynamic_exec64": GENERATED / "behavior_exec64",
		"dynamic_pie64": GENERATED / "behavior_pie64",
		"stripped64": GENERATED / "behavior_stripped64",
		"static64": GENERATED / "behavior_static64",
		"static32": GENERATED / "behavior_static32",
		"pie32": GENERATED / "hello32_pie",
		"shared64": GENERATED / "libshared_profile_e.so",
		"loader64": GENERATED / "shared_loader64",
	}
	commands = {
		"dynamic_exec64": ["gcc", "-O0", "-fno-pie", "-no-pie", "-Wl,--build-id=none", "-o", str(fixtures["dynamic_exec64"]), str(FIXTURE_SRC / "behavior_fixture.c")],
		"dynamic_pie64": ["gcc", "-O0", "-fPIE", "-pie", "-Wl,--build-id=none", "-o", str(fixtures["dynamic_pie64"]), str(FIXTURE_SRC / "behavior_fixture.c")],
		"static64": ["gcc", "-nostdlib", "-static", "-fno-pie", "-no-pie", "-Wl,--build-id=none", "-o", str(fixtures["static64"]), str(FIXTURE_SRC / "behavior_static64.s")],
		"static32": ["gcc", "-m32", "-nostdlib", "-static", "-fno-pie", "-no-pie", "-Wl,--build-id=none", "-o", str(fixtures["static32"]), str(FIXTURE_SRC / "behavior_static32.s")],
		"shared64": ["gcc", "-O0", "-shared", "-fPIC", "-Wl,--build-id=none", "-o", str(fixtures["shared64"]), str(FIXTURE_SRC / "shared_fixture.c")],
		"loader64": ["gcc", "-O0", "-fPIE", "-pie", "-Wl,--build-id=none", "-o", str(fixtures["loader64"]), str(FIXTURE_SRC / "shared_loader.c"), "-ldl"],
	}
	failures: dict[str, str] = {}
	for name, command in commands.items():
		result = setup_command(f"build_{name}", command)
		if result["timed_out"] or result["returncode"] != 0:
			failures[name] = "fixture build failed"
	if "dynamic_exec64" not in failures:
		shutil.copy2(fixtures["dynamic_exec64"], fixtures["stripped64"])
		result = setup_command("strip_dynamic64", ["strip", "--strip-all", str(fixtures["stripped64"])])
		if result["returncode"] != 0:
			failures["stripped64"] = "strip failed"
	else:
		failures["stripped64"] = "base fixture unavailable"
	shutil.copy2(REPO / "hello", fixtures["pie32"])
	for name, path in fixtures.items():
		if name not in failures and not path.is_file():
			failures[name] = "fixture file missing"
	(SETUP_RAW / "summary.json").write_text(
		json.dumps({"fixtures": {name: str(path) for name, path in fixtures.items()}, "failures": failures}, indent=2),
		encoding="utf-8",
	)
	return fixtures, failures


def parse_elf(path: Path) -> dict[str, Any]:
	data = path.read_bytes()
	if len(data) < 16 or data[:4] != b"\x7fELF":
		raise ValueError("not ELF")
	elf_class = data[4]
	if data[5] != 1:
		raise ValueError("not little-endian")
	if elf_class == 1:
		fmt = "<16sHHIIIIIHHHHHH"
		entry_size = 4
		ph_fmt = "<IIIIIIII"
	else:
		if elf_class != 2:
			raise ValueError("unsupported class")
		fmt = "<16sHHIQQQIHHHHHH"
		entry_size = 8
		ph_fmt = "<IIQQQQQQ"
	values = struct.unpack_from(fmt, data, 0)
	keys = ("ident", "type", "machine", "version", "entry", "phoff", "shoff", "flags", "ehsize", "phentsize", "phnum", "shentsize", "shnum", "shstrndx")
	header = dict(zip(keys, values))
	phdrs = []
	for index in range(header["phnum"]):
		offset = header["phoff"] + index * header["phentsize"]
		if offset + struct.calcsize(ph_fmt) > len(data):
			raise ValueError("program header out of range")
		fields = struct.unpack_from(ph_fmt, data, offset)
		if elf_class == 1:
			names = ("type", "offset", "vaddr", "paddr", "filesz", "memsz", "flags", "align")
		else:
			names = ("type", "flags", "offset", "vaddr", "paddr", "filesz", "memsz", "align")
		phdr = dict(zip(names, fields))
		phdr["index"] = index
		phdr["table_offset"] = offset
		phdrs.append(phdr)
	return {"data": data, "class": elf_class, "entry_size": entry_size, "header": header, "phdrs": phdrs}


def eligible_segments(model: dict[str, Any]) -> list[dict[str, Any]]:
	data_len = len(model["data"])
	return [
		phdr for phdr in model["phdrs"]
		if phdr["type"] == 1
		and phdr["offset"] != 0
		and phdr["filesz"] > 0
		and phdr["offset"] <= data_len
		and phdr["filesz"] <= data_len - phdr["offset"]
		and (((phdr["flags"] & 5) == 5) or ((phdr["flags"] & 7) == 4))
	]


def expected_xor(data: bytes, key: int, width: int) -> bytes:
	output = bytearray(data)
	key_bytes = key.to_bytes(8, "little")[:width]
	full = len(output) - (len(output) % width)
	for offset in range(0, full, width):
		for index in range(width):
			output[offset + index] ^= key_bytes[index]
	for offset in range(full, len(output)):
		output[offset] ^= key_bytes[0]
	return bytes(output)


def range_mask(size: int, ranges: list[tuple[int, int]]) -> bytearray:
	mask = bytearray(size)
	for start, end in ranges:
		start = max(0, start)
		end = min(size, end)
		if start < end:
			mask[start:end] = b"\x01" * (end - start)
	return mask


def analyze_structure(input_path: Path, output_path: Path, key_hex: str) -> dict[str, Any]:
	try:
		original = parse_elf(input_path)
		packed = parse_elf(output_path)
	except Exception as exc:
		return {"ok": False, "error": str(exc), "checks": {}}
	segments = eligible_segments(original)
	sc_len = 448 if original["class"] == 1 else 592
	payload_size = sc_len + 48 + 24 * len(segments)
	stub = None
	for segment in segments:
		available = ((segment["memsz"] + 4095) & ~4095) - segment["filesz"]
		if (segment["flags"] & 5) == 5 and available >= payload_size:
			stub = segment
			break
	if stub is None:
		return {"ok": False, "error": "code-derived policy found no stub segment", "checks": {}}
	key = int(key_hex, 16)
	payload_offset = stub["offset"] + stub["filesz"]
	new_entry = stub["vaddr"] + stub["filesz"]
	header_keys = ("ident", "type", "machine", "version", "phoff", "shoff", "flags", "ehsize", "phentsize", "phnum", "shentsize", "shnum", "shstrndx")
	header_preserved = original["class"] == packed["class"] and all(original["header"][key_name] == packed["header"][key_name] for key_name in header_keys)
	entry_correct = packed["header"]["entry"] == new_entry
	phdr_delta_correct = len(original["phdrs"]) == len(packed["phdrs"])
	if phdr_delta_correct:
		for before, after in zip(original["phdrs"], packed["phdrs"]):
			expected = {key_name: value for key_name, value in before.items() if key_name not in ("index", "table_offset")}
			if before["index"] == stub["index"]:
				expected["filesz"] += payload_size
				expected["memsz"] += payload_size
			actual = {key_name: value for key_name, value in after.items() if key_name not in ("index", "table_offset")}
			if expected != actual:
				phdr_delta_correct = False
				break
	load_invariants = True
	for phdr in packed["phdrs"]:
		if phdr["type"] == 1:
			if phdr["filesz"] > phdr["memsz"] or phdr["offset"] + phdr["filesz"] > len(packed["data"]):
				load_invariants = False
			if phdr["align"] > 1 and phdr["offset"] % phdr["align"] != phdr["vaddr"] % phdr["align"]:
				load_invariants = False
	xor_ok = True
	width = 4 if original["class"] == 1 else 8
	for segment in segments:
		start = segment["offset"]
		end = start + segment["filesz"]
		if packed["data"][start:end] != expected_xor(original["data"][start:end], key, width):
			xor_ok = False
			break
	payload = packed["data"][payload_offset:payload_offset + payload_size]
	shellcode = (REPO / "src" / ("shellcode_32.bin" if original["class"] == 1 else "shellcode_64.bin")).read_bytes()
	shellcode_ok = payload[:sc_len] == shellcode
	marker_area = MARKER + b"\x00\x00"
	marker_ok = payload[sc_len:sc_len + 16] == marker_area
	metadata_ok = len(payload) == payload_size
	metadata_values: dict[str, Any] = {}
	if metadata_ok:
		metadata_start = sc_len + 16
		real_entry, payload_entry, payload_key, count = struct.unpack_from("<QQQQ", payload, metadata_start)
		records = [struct.unpack_from("<QQQ", payload, metadata_start + 32 + index * 24) for index in range(count)] if count <= len(segments) else []
		expected_records = []
		for segment in segments:
			protection = (1 if segment["flags"] & 4 else 0) | (2 if segment["flags"] & 2 else 0) | (4 if segment["flags"] & 1 else 0)
			expected_records.append((segment["vaddr"], segment["memsz"], protection))
		metadata_values = {"real_entry": real_entry, "new_entry": payload_entry, "key": payload_key, "count": count, "records": records}
		metadata_ok = (real_entry == original["header"]["entry"] and payload_entry == new_entry and payload_key == key and count == len(segments) and records == expected_records)
	allowed_ranges = [(24, 24 + original["entry_size"]), (payload_offset, payload_offset + payload_size)]
	stub_table = stub["table_offset"]
	if original["class"] == 1:
		allowed_ranges.extend([(stub_table + 16, stub_table + 24)])
	else:
		allowed_ranges.extend([(stub_table + 32, stub_table + 48)])
	allowed_ranges.extend((segment["offset"], segment["offset"] + segment["filesz"]) for segment in segments)
	maximum = max(len(original["data"]), len(packed["data"]))
	mask = range_mask(maximum, allowed_ranges)
	unexpected = []
	unexpected_count = 0
	for offset in range(maximum):
		before = original["data"][offset] if offset < len(original["data"]) else None
		after = packed["data"][offset] if offset < len(packed["data"]) else None
		if before != after and not mask[offset]:
			unexpected_count += 1
			if len(unexpected) < 32:
				unexpected.append(offset)
	checks = {
		"header_preserved": header_preserved,
		"entry_correct": entry_correct,
		"phdr_delta_correct": phdr_delta_correct,
		"load_invariants": load_invariants,
		"xor_ranges_correct": xor_ok,
		"payload_shellcode_correct": shellcode_ok,
		"payload_marker_correct": marker_ok,
		"payload_metadata_correct": metadata_ok,
		"no_unexpected_byte_changes": unexpected_count == 0,
	}
	return {
		"ok": all(checks.values()),
		"checks": checks,
		"class": original["class"],
		"eligible_segment_count": len(segments),
		"stub_phdr_index": stub["index"],
		"payload_offset": payload_offset,
		"payload_size": payload_size,
		"shellcode_size": sc_len,
		"original_entry": original["header"]["entry"],
		"new_entry": new_entry,
		"unexpected_change_count": unexpected_count,
		"unexpected_change_offsets": unexpected,
		"metadata": metadata_values,
	}


def run_packer(case_dir: Path, input_path: Path, key_hex: str, label: str = "pack") -> tuple[dict[str, Any], Path, str, str]:
	work = case_dir / label
	work.mkdir(parents=True)
	before_hash = sha256(input_path)
	result = run_text([str(TARGET), str(input_path), key_hex], cwd=work, timeout_s=5)
	write_text_run(case_dir, label, result)
	after_hash = sha256(input_path)
	output = work / "woody"
	if output.is_file():
		(case_dir / f"{label}.output.sha256").write_text(f"{sha256(output)}  woody\n", encoding="utf-8")
	return result, output, before_hash, after_hash


def pack_failure(result: dict[str, Any], output: Path) -> tuple[str, str] | None:
	if result["timed_out"]:
		return "CRASH", "packer timed out"
	if result["returncode"] is not None and result["returncode"] < 0:
		return "CRASH", f"packer terminated by signal {-result['returncode']}"
	if result["returncode"] != 0 or not output.is_file():
		return "FAIL", "packer did not produce woody"
	return None


def behavior_case(spec: dict[str, Any], fixtures: dict[str, Path], failures: dict[str, str]) -> dict[str, Any]:
	case_dir = PROFILE_RAW / spec["id"]
	case_dir.mkdir(parents=True)
	fixture_name = spec["fixture"]
	base = {"id": spec["id"], "title": spec["title"], "kind": "behavior"}
	if fixture_name in failures:
		return {**base, "classification": "SETUP_FAILURE", "reason": failures[fixture_name]}
	input_path = fixtures[fixture_name]
	pack_result, output, before_hash, after_hash = run_packer(case_dir, input_path, spec.get("key", FIXED_KEY))
	failure = pack_failure(pack_result, output)
	if failure:
		return {**base, "classification": failure[0], "reason": failure[1]}
	structure = analyze_structure(input_path, output, spec.get("key", FIXED_KEY))
	(case_dir / "structure.json").write_text(json.dumps(structure, indent=2), encoding="utf-8")
	run_dir = case_dir / "execution"
	run_dir.mkdir()
	arguments = spec.get("arguments", [])
	stdin_data = spec.get("stdin", b"")
	environment = spec.get("environment")
	original = run_program(input_path, arguments, stdin_data, run_dir, environment)
	packed = run_program(output, arguments, stdin_data, run_dir, environment)
	write_program_run(case_dir, "original", original)
	write_program_run(case_dir, "packed", packed)
	classification, reason, details = compare_behavior(original, packed)
	details.update({"input_hash_before": before_hash, "input_hash_after": after_hash, "structure_ok": structure.get("ok", False)})
	return {**base, "classification": classification, "reason": reason, "details": details}


def shared_object_case(fixtures: dict[str, Path], failures: dict[str, str]) -> dict[str, Any]:
	case_id = "E15"
	case_dir = PROFILE_RAW / case_id
	case_dir.mkdir(parents=True)
	base = {"id": case_id, "title": "shared object loaded through dlopen", "kind": "behavior"}
	for fixture_name in ("shared64", "loader64"):
		if fixture_name in failures:
			return {**base, "classification": "SETUP_FAILURE", "reason": failures[fixture_name]}
	shared = fixtures["shared64"]
	result, output, _before, _after = run_packer(case_dir, shared, FIXED_KEY)
	failure = pack_failure(result, output)
	if failure:
		return {**base, "classification": failure[0], "reason": failure[1]}
	structure = analyze_structure(shared, output, FIXED_KEY)
	(case_dir / "structure.json").write_text(json.dumps(structure, indent=2), encoding="utf-8")
	run_dir = case_dir / "execution"
	run_dir.mkdir()
	original = run_program(fixtures["loader64"], [str(shared)], b"", run_dir)
	packed = run_program(fixtures["loader64"], [str(output)], b"", run_dir)
	write_program_run(case_dir, "original_loader", original)
	write_program_run(case_dir, "packed_loader", packed)
	classification, reason, details = compare_behavior(original, packed, marker_required=False)
	details["structure_ok"] = structure.get("ok", False)
	return {**base, "classification": classification, "reason": reason, "details": details}


def structural_case(spec: dict[str, Any], fixtures: dict[str, Path], failures: dict[str, str]) -> dict[str, Any]:
	case_dir = PROFILE_RAW / spec["id"]
	case_dir.mkdir(parents=True)
	fixture_name = spec["fixture"]
	base = {"id": spec["id"], "title": spec["title"], "kind": "structural"}
	if fixture_name in failures:
		return {**base, "classification": "SETUP_FAILURE", "reason": failures[fixture_name]}
	input_path = fixtures[fixture_name]
	key_hex = spec.get("key", FIXED_KEY)
	result, output, before_hash, after_hash = run_packer(case_dir, input_path, key_hex)
	failure = pack_failure(result, output)
	if failure:
		return {**base, "classification": failure[0], "reason": failure[1]}
	structure = analyze_structure(input_path, output, key_hex)
	(case_dir / "structure.json").write_text(json.dumps(structure, indent=2), encoding="utf-8")
	focus = spec["focus"]
	if focus == "header_phdr":
		wanted = ("header_preserved", "entry_correct", "phdr_delta_correct", "load_invariants")
		passed = all(structure.get("checks", {}).get(name, False) for name in wanted)
		reason = "ELF header/PHDR delta matched" if passed else "ELF header/PHDR invariant failed"
	elif focus == "xor":
		wanted = ("xor_ranges_correct", "no_unexpected_byte_changes")
		passed = all(structure.get("checks", {}).get(name, False) for name in wanted)
		reason = "encrypted ranges and global byte-delta allowlist matched" if passed else "encrypted range/delta mismatch"
	elif focus == "payload":
		wanted = ("payload_shellcode_correct", "payload_marker_correct", "payload_metadata_correct")
		passed = all(structure.get("checks", {}).get(name, False) for name in wanted)
		reason = "payload shellcode/marker/metadata matched" if passed else "payload layout mismatch"
	elif focus == "input_integrity":
		passed = before_hash == after_hash
		reason = "input hash unchanged" if passed else "input hash changed"
	elif focus == "mode_format":
		mode = stat.S_IMODE(output.stat().st_mode)
		original_model = parse_elf(input_path)
		packed_model = parse_elf(output)
		passed = bool(mode & 0o111) and original_model["class"] == packed_model["class"] and original_model["header"]["type"] == packed_model["header"]["type"] and original_model["header"]["machine"] == packed_model["header"]["machine"]
		reason = f"output executable mode {mode:o} and ELF identity preserved" if passed else "output mode/format mismatch"
	else:
		raise ValueError(focus)
	return {**base, "classification": "PASS" if passed else "FAIL", "reason": reason, "details": {"structure_ok": structure.get("ok", False), "checks": structure.get("checks", {})}}


def determinism_case(fixtures: dict[str, Path], failures: dict[str, str]) -> dict[str, Any]:
	case_id = "E20"
	case_dir = PROFILE_RAW / case_id
	case_dir.mkdir(parents=True)
	base = {"id": case_id, "title": "same input and key produce identical output", "kind": "determinism"}
	if "dynamic_exec64" in failures:
		return {**base, "classification": "SETUP_FAILURE", "reason": failures["dynamic_exec64"]}
	first_result, first_output, _before1, _after1 = run_packer(case_dir, fixtures["dynamic_exec64"], FIXED_KEY, "pack_first")
	second_result, second_output, _before2, _after2 = run_packer(case_dir, fixtures["dynamic_exec64"], FIXED_KEY, "pack_second")
	for result, output in ((first_result, first_output), (second_result, second_output)):
		failure = pack_failure(result, output)
		if failure:
			return {**base, "classification": failure[0], "reason": failure[1]}
	first_hash = sha256(first_output)
	second_hash = sha256(second_output)
	passed = first_hash == second_hash
	return {**base, "classification": "PASS" if passed else "FAIL", "reason": "output hashes matched" if passed else "output hashes differed", "details": {"first_sha256": first_hash, "second_sha256": second_hash}}


def specs() -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
	behavior = [
		{"id": "E01", "title": "ELF64 ET_EXEC default behavior", "fixture": "dynamic_exec64"},
		{"id": "E02", "title": "ELF64 PIE default behavior", "fixture": "dynamic_pie64"},
		{"id": "E03", "title": "ELF64 static syscall behavior", "fixture": "static64"},
		{"id": "E04", "title": "ELF32 static syscall behavior", "fixture": "static32"},
		{"id": "E05", "title": "ELF32 PIE tracked hello behavior", "fixture": "pie32"},
		{"id": "E06", "title": "stripped ELF64 behavior", "fixture": "stripped64"},
		{"id": "E07", "title": "argument preservation", "fixture": "dynamic_exec64", "arguments": ["argv", "alpha", "two words", ""]},
		{"id": "E08", "title": "stdout preservation", "fixture": "dynamic_exec64", "arguments": ["stdout"]},
		{"id": "E09", "title": "stderr preservation", "fixture": "dynamic_exec64", "arguments": ["stderr"]},
		{"id": "E10", "title": "exit status preservation", "fixture": "dynamic_exec64", "arguments": ["exit", "7"]},
		{"id": "E11", "title": "binary stdin preservation", "fixture": "dynamic_exec64", "arguments": ["stdin"], "stdin": b"stdin:\x00binary\nline2\n"},
		{"id": "E12", "title": "environment-derived behavior", "fixture": "dynamic_exec64", "arguments": ["env"], "environment": {"PACKER_TEST_ENV": "profile-e-value"}},
		{"id": "E13", "title": "all-zero explicit key behavior", "fixture": "dynamic_exec64", "key": "0000000000000000"},
		{"id": "E14", "title": "all-one explicit key behavior", "fixture": "dynamic_exec64", "key": "ffffffffffffffff"},
	]
	structural = [
		{"id": "E16", "title": "ELF64 header and PHDR delta", "fixture": "dynamic_exec64", "focus": "header_phdr"},
		{"id": "E17", "title": "ELF32 header and PHDR delta", "fixture": "static32", "focus": "header_phdr"},
		{"id": "E18", "title": "XOR ranges and byte-delta allowlist", "fixture": "dynamic_pie64", "focus": "xor"},
		{"id": "E19", "title": "payload shellcode and metadata", "fixture": "static64", "focus": "payload"},
		{"id": "E21", "title": "original input integrity", "fixture": "dynamic_exec64", "focus": "input_integrity"},
		{"id": "E22", "title": "output executable mode and ELF identity", "fixture": "dynamic_pie64", "focus": "mode_format"},
	]
	return behavior, structural


def write_results(results: list[dict[str, Any]], fixtures: dict[str, Path], failures: dict[str, str]) -> None:
	observed = collections.Counter(result["classification"] for result in results)
	counts = {category: observed.get(category, 0) for category in ("PASS", "PARTIAL", "FAIL", "CRASH", "SETUP_FAILURE")}
	payload = {
		"profile": "PROFILE E extension",
		"target_sha256": sha256(TARGET),
		"fixture_count": len(fixtures),
		"case_count": len(results),
		"counts": counts,
		"normalization": ["same synthetic argv[0]", "remove exactly one leading 14-byte WOODY marker from packed stdout"],
		"setup_failures": failures,
		"results": results,
	}
	RESULTS_JSON.write_text(json.dumps(payload, indent=2), encoding="utf-8")
	lines = [
		"# PROFILE E baseline results",
		"",
		f"Fixtures: {len(fixtures)}; cases: {len(results)}; " + ", ".join(f"{name}={value}" for name, value in counts.items()),
		"",
		"| ID | Classification | Kind | Case | Evidence summary |",
		"|---|---|---|---|---|",
	]
	for result in results:
		reason = result["reason"].replace("|", "\\|").replace("\n", " ")
		lines.append(f"| {result['id']} | {result['classification']} | {result['kind']} | {result['title']} | {reason} |")
	lines.extend(["", "Raw evidence is stored under `raw/profile_e/<case-id>/`."])
	RESULTS_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
	if not TARGET.is_file():
		raise SystemExit("isolated target binary is missing")
	fixtures, failures = build_fixtures()
	behavior_specs, structural_specs = specs()
	results = [behavior_case(spec, fixtures, failures) for spec in behavior_specs]
	results.append(shared_object_case(fixtures, failures))
	results.extend(structural_case(spec, fixtures, failures) for spec in structural_specs[:4])
	results.append(determinism_case(fixtures, failures))
	results.extend(structural_case(spec, fixtures, failures) for spec in structural_specs[4:])
	results.sort(key=lambda result: int(result["id"][1:]))
	write_results(results, fixtures, failures)
	print(json.dumps(collections.Counter(result["classification"] for result in results), sort_keys=True))
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
