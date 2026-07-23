"""
Demo Rotator Script
Continuously rotates the attached entity around its Y axis.
Demonstrates reflected transform field access from Python.
"""
import z1
import math

class Rotator(z1.Script):
    """Simple rotating script using reflected transform.rotation field."""

    def __init__(self, speed=45.0):
        super().__init__()
        self._speed = speed  # degrees per second

    def on_attach(self):
        z1.log_info(f"[Rotator] Attached, speed={self._speed} deg/s")

    def on_update(self, delta):
        if not self.entity or not self.entity.transform:
            return
        t = self.entity.transform
        t.rotation = z1.Vec3(
            t.rotation.x,
            t.rotation.y + self._speed * delta,
            t.rotation.z
        )

    def on_detach(self):
        pass
