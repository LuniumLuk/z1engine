import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from commands import benchmark


class BenchmarkIntegrationTests(unittest.TestCase):
	def setUp(self):
		self.root = Path(__file__).resolve().parents[2]
		self.suites_dir = self.root / "dev" / "tests" / "fixtures" / "benchmark_suites"
		self.baselines_dir = self.root / "dev" / "tests" / "fixtures" / "benchmark_baselines"
		self.temp_reports = Path(tempfile.mkdtemp(prefix="z1-bench-reports-"))

	def tearDown(self):
		for p in sorted(self.temp_reports.glob("**/*"), reverse=True):
			if p.is_file():
				p.unlink(missing_ok=True)
			elif p.is_dir():
				p.rmdir()
		if self.temp_reports.exists():
			self.temp_reports.rmdir()

	def test_fixture_pass(self):
		rc = benchmark.main([
			"--suite", "fixture-pass",
			"--suites-dir", str(self.suites_dir),
			"--baseline-dir", str(self.baselines_dir),
			"--report-dir", str(self.temp_reports),
		])
		self.assertEqual(rc, 0)
		latest = self.temp_reports / "benchmark-latest.json"
		self.assertTrue(latest.exists())
		data = json.loads(latest.read_text(encoding="utf-8"))
		self.assertEqual(data["status"], "ok")

	def test_fixture_fail(self):
		rc = benchmark.main([
			"--suite", "fixture-fail",
			"--suites-dir", str(self.suites_dir),
			"--baseline-dir", str(self.baselines_dir),
			"--report-dir", str(self.temp_reports),
		])
		self.assertEqual(rc, 2)
		latest = self.temp_reports / "benchmark-latest.json"
		self.assertTrue(latest.exists())
		data = json.loads(latest.read_text(encoding="utf-8"))
		self.assertEqual(data["status"], "fail")
		self.assertEqual(data["failed"], 1)


if __name__ == "__main__":
	unittest.main()
