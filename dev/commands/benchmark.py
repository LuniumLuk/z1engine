"""benchmark -- Run benchmark suites and compare against baselines."""

import argparse
import datetime
import json
import platform
import shutil
from pathlib import Path

from commands._common import (
	EXIT_OK,
	EXIT_CONFIG_ERROR,
	EXIT_VALIDATION_ERROR,
	Timer,
	make_result,
	normalize_config,
	print_fail,
	print_info,
	print_ok,
	print_run,
	repo_root,
	run_subprocess,
)


def _percentile(values, pct):
	if not values:
		return None
	ordered = sorted(values)
	if len(ordered) == 1:
		return ordered[0]
	k = (len(ordered) - 1) * (pct / 100.0)
	f = int(k)
	c = min(f + 1, len(ordered) - 1)
	if f == c:
		return ordered[f]
	return ordered[f] + (ordered[c] - ordered[f]) * (k - f)


def _aggregate(values, agg):
	if not values:
		return None
	a = (agg or "p50").lower()
	if a == "p95":
		return _percentile(values, 95.0)
	return _percentile(values, 50.0)


def _read_json(path):
	with path.open("r", encoding="utf-8") as f:
		return json.load(f)


def _load_policy(path):
	p = Path(path)
	if not p.exists():
		return {}
	return _read_json(p)


def _write_benchmark_report(root, report, report_dir):
	report_dir = Path(report_dir)
	if not report_dir.is_absolute():
		report_dir = Path(root) / report_dir
	report_dir.mkdir(parents=True, exist_ok=True)
	now = datetime.datetime.now(datetime.UTC)
	ts = now.strftime("%Y%m%dT%H%M%SZ")
	timestamped = report_dir / f"benchmark-{ts}.json"
	latest = report_dir / "benchmark-latest.json"
	with timestamped.open("w", encoding="utf-8") as f:
		json.dump(report, f, indent=2)
	with latest.open("w", encoding="utf-8") as f:
		json.dump(report, f, indent=2)
	return str(timestamped), str(latest)


def _cleanup_workspace(root, policy):
	h = policy.get("workspaceHygiene", {}) if policy else {}
	if not h.get("enabled", False):
		return
	if not h.get("cleanupAfterRun", False):
		return
	for pattern in h.get("cleanupPaths", []):
		for p in root.glob(pattern):
			try:
				if p.is_dir():
					shutil.rmtree(p, ignore_errors=True)
				elif p.exists():
					p.unlink(missing_ok=True)
			except Exception:
				pass


def _resolve_working_directory(root, suite, case):
	working_dir = case.get("workingDirectory", suite.get("workingDirectory", "."))
	working_dir = str(working_dir).replace("{repo_root}", str(root))
	p = Path(working_dir)
	if not p.is_absolute():
		p = root / p
	return p


def _resolve_command(root, config, suite, case):
	template_vars = {
		"repo_root": str(root),
		"config": config,
		"case_id": case.get("id", "case"),
	}

	if suite.get("command"):
		cmd = []
		for part in suite.get("command", []):
			cmd.append(str(part).format(**template_vars))
		for part in case.get("args", []):
			cmd.append(str(part).format(**template_vars))
		return cmd

	binary = Path(str(suite.get("binary", "")).format(**template_vars))
	if not binary.is_absolute():
		binary = root / binary
	cmd = [str(binary)]
	for part in case.get("args", []):
		cmd.append(str(part).format(**template_vars))
	return cmd


def _resolve_trace_source(root, working_dir, suite, case, metric):
	source = (
		metric.get("source")
		or case.get("traceSource")
		or suite.get("traceSource")
		or "profile-run.json"
	)
	source = str(source).replace("{repo_root}", str(root)).replace("{working_dir}", str(working_dir))
	p = Path(source)
	if p.is_absolute():
		return p
	if str(source).startswith("."):
		return (working_dir / p).resolve()
	return (working_dir / p).resolve()


def _classify_regression(current, base, threshold_pct, lower_is_better):
	if base is None or current is None:
		return None, None
	if base == 0:
		delta_pct = 0.0 if current == 0 else 9999.0
	else:
		delta_pct = ((current - base) / base) * 100.0
	if lower_is_better:
		return delta_pct > threshold_pct, delta_pct
	return delta_pct < -threshold_pct, delta_pct


def _extract_metric(trace_json, metric):
	events = trace_json.get("traceEvents", [])
	name_contains = metric.get("traceNameContains", "").lower().strip()
	selected = []
	for ev in events:
		if ev.get("ph") != "X":
			continue
		dur = ev.get("dur")
		if not isinstance(dur, (int, float)):
			continue
		if name_contains:
			name = str(ev.get("name", "")).lower()
			if name_contains not in name:
				continue
		selected.append(float(dur))

	if not selected:
		return None

	agg = metric.get("aggregation", "p50")
	value_us = _aggregate(selected, agg)
	if value_us is None:
		return None

	metric_name = str(metric.get("name", "")).lower()
	if "_ms_" in metric_name or metric_name.endswith("_ms"):
		return value_us / 1000.0
	return value_us


def _discover_suites(suites_dir, requested):
	all_files = sorted(Path(suites_dir).glob("*.json"))
	suites = []
	for f in all_files:
		data = _read_json(f)
		name = data.get("suite", f.stem)
		if requested and name not in requested:
			continue
		if not data.get("enabled", True):
			continue
		data["_file"] = str(f)
		suites.append(data)
	if requested:
		names = {s.get("suite") for s in suites}
		missing = [r for r in requested if r not in names]
		if missing:
			return None, missing
	return suites, []


def _load_baseline(baseline_dir, suite_name):
	path = Path(baseline_dir) / f"{suite_name}.baseline.json"
	if not path.exists():
		return None, path
	return _read_json(path), path


def _write_baseline(path, suite_name, config, case_values):
	path.parent.mkdir(parents=True, exist_ok=True)
	data = {
		"suite": suite_name,
		"config": config,
		"generatedAt": "manual-update",
		"cases": case_values,
	}
	with path.open("w", encoding="utf-8") as f:
		json.dump(data, f, indent=2)


def main(argv=None):
	parser = argparse.ArgumentParser(
		prog="z1 benchmark",
		description="Run benchmark suites and compare against baselines.",
	)
	parser.add_argument("--suite", action="append", default=[],
		help="Suite name to run (repeatable)")
	parser.add_argument("--config", default="Hybrid",
		help="Build config (default: Hybrid)")
	parser.add_argument("--update-baseline", action="store_true",
		help="Update baseline files with measured values")
	parser.add_argument("--suites-dir", default="dev/benchmark/suites",
		help="Benchmark suite definition directory")
	parser.add_argument("--baseline-dir", default=None,
		help="Baseline directory (defaults to dev/benchmark/baselines/<config>)")
	parser.add_argument("--policy", default="dev/validation/policy.json",
		help="Validation policy file path")
	parser.add_argument("--report-dir", default="dev/validation/reports",
		help="Report output directory (default: dev/validation/reports)")
	args = parser.parse_args(argv)

	config = normalize_config(args.config) or "Hybrid"
	root = repo_root()
	suites_dir = root / args.suites_dir
	baseline_dir = root / (args.baseline_dir or f"dev/benchmark/baselines/{config.lower()}")
	policy = _load_policy(root / args.policy)

	timer = Timer()
	print_run("benchmark")

	if not suites_dir.exists():
		print_fail(f"suites dir not found: {suites_dir}")
		return make_result("fail", "benchmark", exit_code=EXIT_CONFIG_ERROR, detail="missing suites dir")

	suites, missing = _discover_suites(suites_dir, args.suite)
	if missing:
		print_fail(f"unknown suite(s): {', '.join(missing)}")
		return make_result("fail", "benchmark", exit_code=EXIT_CONFIG_ERROR, detail="unknown suite")
	if not suites:
		print_fail("no benchmark suites discovered")
		return make_result("fail", "benchmark", exit_code=EXIT_CONFIG_ERROR, detail="no suites")

	passed = 0
	failed = 0
	warned = 0
	suite_outcomes = []

	for suite in suites:
		suite_name = suite.get("suite", "unknown")
		print_info(f"Suite: {suite_name}")

		if not suite.get("command"):
			binary = Path(str(suite.get("binary", "")))
			if not binary.is_absolute():
				binary = root / binary
			if not binary.exists():
				print_fail(f"missing benchmark binary: {binary}")
				failed += 1
				suite_outcomes.append({"suite": suite_name, "status": "fail", "detail": "missing binary"})
				continue

		warmup_runs = int(suite.get("warmupRuns", 1))
		measured_runs = int(suite.get("measuredRuns", 3))
		metrics = suite.get("metrics", [])
		cases = suite.get("cases", [])
		case_values = {}
		suite_fail = False
		suite_regressions = []

		for case in cases:
			case_id = case.get("id", "case")
			working_dir = _resolve_working_directory(root, suite, case)
			working_dir.mkdir(parents=True, exist_ok=True)
			cmd = _resolve_command(root, config, suite, case)
			metric_runs = {m.get("name", "metric"): [] for m in metrics}

			for _ in range(warmup_runs):
				_cleanup_workspace(root, policy)
				rc, _, stderr = run_subprocess(cmd, cwd=str(working_dir), timeout=180)
				if rc != 0:
					print_fail(f"{suite_name}/{case_id} warmup failed: exit {rc}")
					if stderr.strip():
						print_info(stderr.strip().splitlines()[0])
					suite_fail = True
					break
			if suite_fail:
				break

			for _ in range(measured_runs):
				_cleanup_workspace(root, policy)
				rc, _, stderr = run_subprocess(cmd, cwd=str(working_dir), timeout=240)
				if rc != 0:
					print_fail(f"{suite_name}/{case_id} run failed: exit {rc}")
					if stderr.strip():
						print_info(stderr.strip().splitlines()[0])
					suite_fail = True
					break

				for metric in metrics:
					source_path = _resolve_trace_source(root, working_dir, suite, case, metric)
					if not source_path.exists():
						suite_fail = True
						print_fail(f"missing metric source: {source_path}")
						break
					try:
						trace = _read_json(source_path)
					except Exception:
						suite_fail = True
						print_fail(f"failed to parse trace: {source_path}")
						break
					value = _extract_metric(trace, metric)
					if value is None:
						suite_fail = True
						print_fail(f"no samples for metric {metric.get('name', 'metric')}")
						break
					metric_runs[metric.get("name", "metric")].append(value)
				if suite_fail:
					break

			if suite_fail:
				break

			case_values[case_id] = {}
			for metric in metrics:
				mname = metric.get("name", "metric")
				case_values[case_id][mname] = _aggregate(metric_runs[mname], "p50")

		if suite_fail:
			failed += 1
			suite_outcomes.append({"suite": suite_name, "status": "fail", "detail": "execution failure"})
			_cleanup_workspace(root, policy)
			continue

		baseline, baseline_path = _load_baseline(baseline_dir, suite_name)
		if args.update_baseline:
			_write_baseline(baseline_path, suite_name, config, case_values)
			print_ok(f"updated baseline: {baseline_path}")
			passed += 1
			suite_outcomes.append({"suite": suite_name, "status": "ok", "updated_baseline": True})
			_cleanup_workspace(root, policy)
			continue

		if baseline is None:
			missing_policy = policy.get("gates", {}).get("missingBaseline", "warn")
			if missing_policy == "warn":
				warned += 1
				print_warn = print_info
				print_warn(f"missing baseline (warn): {baseline_path}")
				suite_outcomes.append({"suite": suite_name, "status": "warn", "detail": "missing baseline"})
			else:
				print_fail(f"missing baseline: {baseline_path}")
				failed += 1
				suite_outcomes.append({"suite": suite_name, "status": "fail", "detail": "missing baseline"})
			_cleanup_workspace(root, policy)
			continue

		baseline_cases = baseline.get("cases", {})
		for case_id, values in case_values.items():
			base_values = baseline_cases.get(case_id, {})
			for metric in metrics:
				mname = metric.get("name", "metric")
				threshold_pct = float(metric.get("thresholdPercent", 0.0))
				lower_is_better = bool(metric.get("lowerIsBetter", True))
				current = values.get(mname)
				base = base_values.get(mname)
				if base is None or current is None:
					suite_regressions.append({
						"case": case_id,
						"metric": mname,
						"reason": "missing baseline metric",
					})
					continue

				is_regression, delta_pct = _classify_regression(current, base, threshold_pct, lower_is_better)
				if is_regression is None:
					continue

				if is_regression:
					suite_regressions.append({
						"case": case_id,
						"metric": mname,
						"baseline": base,
						"current": current,
						"deltaPercent": round(delta_pct, 3),
						"thresholdPercent": threshold_pct,
					})

		if suite_regressions:
			failed += 1
			print_fail(f"{suite_name}: {len(suite_regressions)} regression(s)")
			suite_outcomes.append({
				"suite": suite_name,
				"status": "fail",
				"regressions": suite_regressions,
			})
		else:
			passed += 1
			print_ok(f"{suite_name}: passed")
			suite_outcomes.append({"suite": suite_name, "status": "ok"})

		_cleanup_workspace(root, policy)

	elapsed = timer.elapsed()
	status = "ok" if failed == 0 else "fail"
	exit_code = EXIT_OK if failed == 0 else EXIT_VALIDATION_ERROR
	if failed > 0:
		print_fail(f"benchmark failed ({failed} failed, {passed} passed, {elapsed})")
	else:
		print_ok(f"benchmark completed ({passed} passed, {elapsed})")

	report = {
		"schemaVersion": 1,
		"command": "benchmark",
		"status": status,
		"timestampUtc": datetime.datetime.now(datetime.UTC).isoformat().replace("+00:00", "Z"),
		"environment": {
			"platform": platform.platform(),
			"pythonVersion": platform.python_version(),
			"config": config,
			"repoRoot": str(root),
		},
		"requestedSuites": args.suite,
		"baselineDir": str(baseline_dir),
		"suites": suite_outcomes,
		"passed": passed,
		"failed": failed,
		"warned": warned,
		"elapsed": elapsed,
	}
	report_file, report_latest = _write_benchmark_report(root, report, args.report_dir)
	print_info(f"Benchmark report: {report_latest}")

	return make_result(
		status,
		"benchmark",
		exit_code=exit_code,
		passed=passed,
		failed=failed,
		warned=warned,
		total=passed + failed,
		suites=suite_outcomes,
		report_file=report_file,
		report_latest=report_latest,
		elapsed=elapsed,
	)
