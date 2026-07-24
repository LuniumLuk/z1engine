"""
Kinematic Platform Script
Oscillates the entity back and forth along a configurable axis.
Demonstrates script-driven kinematic physics bodies.
"""
import z1
import math

class KinematicPlatform(z1.Script):
	"""Moves the attached entity along an axis in a sine-wave pattern."""

	def __init__(self, axis='x', amplitude=3.0, speed=2.0):
		super().__init__()
		self._axis = axis
		self._amplitude = amplitude
		self._speed = speed
		self._elapsed = 0.0
		self._start_pos = None

	def on_attach(self):
		z1.log_info(f"[KinematicPlatform] Attached, axis={self._axis} "
		            f"amplitude={self._amplitude} speed={self._speed}")

	def on_start(self):
		if self.entity and self.entity.transform:
			t = self.entity.transform
			self._start_pos = z1.Vec3(t.location.x, t.location.y, t.location.z)

	def on_update(self, delta):
		if not self.entity or not self.entity.transform or self._start_pos is None:
			return
		self._elapsed += delta
		offset = math.sin(self._elapsed * self._speed) * self._amplitude
		t = self.entity.transform
		if self._axis == 'x':
			t.location = z1.Vec3(self._start_pos.x + offset, self._start_pos.y, self._start_pos.z)
		elif self._axis == 'y':
			t.location = z1.Vec3(self._start_pos.x, self._start_pos.y + offset, self._start_pos.z)
		elif self._axis == 'z':
			t.location = z1.Vec3(self._start_pos.x, self._start_pos.y, self._start_pos.z + offset)

	def on_detach(self):
		pass
