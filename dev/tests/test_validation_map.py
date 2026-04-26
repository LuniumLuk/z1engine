import unittest
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from commands.validation_map import select_suites


class ValidationMapTests(unittest.TestCase):
	def test_scoped_selection(self):
		suite_map = {
			"default": {"tests": ["test_binary_file"], "benchmarks": ["runtime-core"]},
			"pathRules": [
				{
					"matchAny": ["engine/runtime/source/render/"],
					"tests": ["test_render_graph"],
					"benchmarks": ["rendering-pipeline"],
				}
			],
			"escalationRules": [],
			"fullMode": {"tests": ["all-tests"], "benchmarks": ["all-bench"]},
		}

		selection = select_suites(["engine/runtime/source/render/renderer/pipeline.cpp"], suite_map)
		self.assertFalse(selection["full_mode"])
		self.assertEqual(selection["tests"], ["test_binary_file", "test_render_graph"])
		self.assertEqual(selection["benchmarks"], ["rendering-pipeline", "runtime-core"])

	def test_full_mode_escalation(self):
		suite_map = {
			"default": {"tests": ["test_binary_file"], "benchmarks": ["runtime-core"]},
			"pathRules": [],
			"escalationRules": [
				{"matchAny": ["engine/runtime/source/render/rhi/"], "mode": "full"}
			],
			"fullMode": {
				"tests": ["test_a", "test_b"],
				"benchmarks": ["bench_a", "bench_b"],
			},
		}

		selection = select_suites(["engine/runtime/source/render/rhi/opengl_buffer.cpp"], suite_map)
		self.assertTrue(selection["full_mode"])
		self.assertEqual(selection["tests"], ["test_a", "test_b"])
		self.assertEqual(selection["benchmarks"], ["bench_a", "bench_b"])

	def test_empty_map_returns_empty(self):
		selection = select_suites(["engine/runtime/source/core/application.cpp"], None)
		self.assertEqual(selection["tests"], [])
		self.assertEqual(selection["benchmarks"], [])
		self.assertFalse(selection["full_mode"])


if __name__ == "__main__":
	unittest.main()
