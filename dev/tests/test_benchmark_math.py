import unittest
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from commands.benchmark import _aggregate, _percentile


class BenchmarkMathTests(unittest.TestCase):
	def test_percentile_basic(self):
		values = [1.0, 2.0, 3.0, 4.0, 5.0]
		self.assertAlmostEqual(_percentile(values, 50.0), 3.0)
		self.assertAlmostEqual(_percentile(values, 95.0), 4.8)

	def test_aggregate_modes(self):
		values = [10.0, 20.0, 30.0, 40.0, 50.0]
		self.assertAlmostEqual(_aggregate(values, "p50"), 30.0)
		self.assertAlmostEqual(_aggregate(values, "p95"), 48.0)

	def test_empty_values(self):
		self.assertIsNone(_percentile([], 50.0))
		self.assertIsNone(_aggregate([], "p50"))


if __name__ == "__main__":
	unittest.main()
