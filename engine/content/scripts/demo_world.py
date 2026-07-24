"""
Demo World Script
Spawns a grid of cubes with Rotator scripts, adjusts sun settings,
and demonstrates Python scripting API capabilities using reflected fields.
"""
import z1
import math
import time

class DemoWorld(z1.Script):
	"""Manages a demo world: spawns entities, tweaks sun, shows lifecycle."""

	def __init__(self):
		super().__init__()
		self._spawned = []
		self._elapsed = 0.0
		self._sun_angle = 0.0

	def on_attach(self):
		z1.log_info("[DemoWorld] Attached! Creating demo environment...")

	def on_start(self):
		"""Spawn entities when the scene starts."""
		z1.log_info("[DemoWorld] Starting demo world setup...")

		# 1. Spawn a grid of cubes on the ground
		z1.log_info("[DemoWorld] Spawning cube grid...")
		for row in range(-2, 3):
			for col in range(-2, 3):
				name = f"Cube_{row+2}_{col+2}"
				try:
					ent = z1.scene.create_entity(name)
					# Place in a grid
					if ent.transform:
						ent.transform.location = z1.Vec3(
							float(col * 3),
							0.0,
							float(row * 3)
						)
						ent.transform.scale = z1.Vec3(0.5, 0.5, 0.5)
					# Add static mesh using engine built-in cube mesh
					ent.add_static_mesh("mesh/SM_Cube")
					self._spawned.append(ent)
				except Exception as e:
					z1.log_warn(f"[DemoWorld] Failed to spawn {name}: {e}")

		z1.log_info(f"[DemoWorld] Spawned {len(self._spawned)} cubes in a 5x5 grid.")

		# 2. Spawn a large central sphere
		try:
			center = z1.scene.create_entity("CenterSphere")
			if center.transform:
				center.transform.location = z1.Vec3(0.0, 2.0, 0.0)
				center.transform.scale = z1.Vec3(1.2, 1.2, 1.2)
			center.add_static_mesh("mesh/SM_Sphere")
			self._spawned.append(center)
			z1.log_info("[DemoWorld] Spawned center sphere.")
		except Exception as e:
			z1.log_warn(f"[DemoWorld] Failed to spawn center sphere: {e}")

		# 3. Spawn cylinder pillars
		for i, (x, z) in enumerate([(-5, -5), (5, -5), (-5, 5), (5, 5)]):
			try:
				pillar = z1.scene.create_entity(f"Pillar_{i}")
				if pillar.transform:
					pillar.transform.location = z1.Vec3(float(x), 0.0, float(z))
					pillar.transform.scale = z1.Vec3(0.4, 2.0, 0.4)
				pillar.add_static_mesh("mesh/SM_Cylinder")
				self._spawned.append(pillar)
			except Exception as e:
				z1.log_warn(f"[DemoWorld] Failed to spawn pillar {i}: {e}")

		z1.log_info("[DemoWorld] Demo world setup complete!")

	def on_update(self, delta):
		"""Animate sun direction to create a day cycle effect."""
		self._elapsed += delta
		self._sun_angle += delta * 15.0  # 15 degrees per second

		# Modify GlobalSettings via z1.globals (reflected fields)
		try:
			g = z1.globals
			# Animate sun direction in a circle
			rad = math.radians(self._sun_angle)
			g.sun_direction = z1.Vec3(
				math.cos(rad) * 0.7,
				0.6,  # positive Y: sun is above
				math.sin(rad) * 0.7
			)
			# Vary sun intensity slightly
			g.sun_intensity = 2.5 + math.sin(self._sun_angle * 0.5) * 0.5
		except Exception as e:
			z1.log_warn(f"[DemoWorld] Sun update error: {e}")

	def on_destroy(self):
		"""Clean up spawned entities."""
		z1.log_info(f"[DemoWorld] Destroying demo world ({len(self._spawned)} entities)...")
		for ent in self._spawned:
			try:
				z1.scene.destroy_entity(ent)
			except:
				pass
		self._spawned.clear()

	def on_detach(self):
		z1.log_info("[DemoWorld] Detached.")
