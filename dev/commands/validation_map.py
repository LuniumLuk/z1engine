"""Validation suite-map selection helpers."""

import json
from pathlib import Path


def _norm(path):
	return str(path).replace("\\", "/").lower()


def load_suite_map(path):
	p = Path(path)
	if not p.exists():
		return None
	with p.open("r", encoding="utf-8") as f:
		return json.load(f)


def _matches_any(path, patterns):
	for pat in patterns:
		if _norm(pat) in path:
			return True
	return False


def select_suites(changed_files, suite_map):
	if not suite_map:
		return {
			"tests": [],
			"benchmarks": [],
			"full_mode": False,
		}

	tests = set(suite_map.get("default", {}).get("tests", []))
	benchmarks = set(suite_map.get("default", {}).get("benchmarks", []))
	full_mode = False

	norm_changed = [_norm(p) for p in changed_files]

	for rule in suite_map.get("pathRules", []):
		patterns = rule.get("matchAny", [])
		if any(_matches_any(f, patterns) for f in norm_changed):
			tests.update(rule.get("tests", []))
			benchmarks.update(rule.get("benchmarks", []))

	for rule in suite_map.get("escalationRules", []):
		patterns = rule.get("matchAny", [])
		if any(_matches_any(f, patterns) for f in norm_changed):
			if rule.get("mode") == "full":
				full_mode = True
				break

	if full_mode:
		tests = set(suite_map.get("fullMode", {}).get("tests", []))
		benchmarks = set(suite_map.get("fullMode", {}).get("benchmarks", []))

	return {
		"tests": sorted(tests),
		"benchmarks": sorted(benchmarks),
		"full_mode": full_mode,
	}
