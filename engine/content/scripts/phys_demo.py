"""
Physics Demo Spawner
Spawns all dynamic and kinematic physics objects at runtime so the
scene file only contains static geometry. This prevents objects
from falling and being saved at ground level in the editor.
"""
import z1
import math


class PhysDemo(z1.Script):
	"""Spawns a physics playground: walls, platforms, boxes, spheres, ramp."""

	def __init__(self):
		super().__init__()
		self._spawned = []

	def on_attach(self):
		z1.log_info("[PhysDemo] Attached! Spawning physics playground...")

	def on_start(self):
		mat = "f2142c5c-3ec1-480a-b9e6-3856e983d39b"

		# --- Walls (static) ---
		walls = [
			("Wall Back",  (0, 0, -8),    (16, 2, 0.5),   (8, 2, 0.25)),
			("Wall Left",  (-8, 0, 0),    (0.5, 2, 16),   (0.25, 2, 8)),
			("Wall Right", (8, 0, 0),     (0.5, 2, 16),   (0.25, 2, 8)),
			("Wall Front", (0, 0, 8),     (16, 2, 0.5),   (8, 2, 0.25)),
		]
		for name, pos, scale, half in walls:
			e = z1.scene.create_entity(name)
			e.transform.location = z1.Vec3(*pos)
			e.transform.scale = z1.Vec3(*scale)
			e.add_static_mesh("mesh/SM_Cube")
			e.add_collider(1, *half)       # Box
			e.add_physics(0, 1, False, 0)  # Static
			self._spawned.append(e)

		# --- Kinematic rotating platform ---
		rp = z1.scene.create_entity("Rotating Platform")
		rp.transform.location = z1.Vec3(3, 0.5, -3)
		rp.transform.scale = z1.Vec3(3, 0.3, 3)
		rp.add_static_mesh("mesh/SM_Cube")
		rp.add_collider(1, 1.5, 0.15, 1.5)     # Box
		rp.add_physics(1, 1, False, 0)           # Kinematic
		rp.add_script("scripts.rotator", "Rotator")
		self._spawned.append(rp)

		# --- Kinematic sliding platform ---
		sp = z1.scene.create_entity("Sliding Platform")
		sp.transform.location = z1.Vec3(-3, 1.2, -3)
		sp.transform.scale = z1.Vec3(1.5, 0.3, 3)
		sp.add_static_mesh("mesh/SM_Cube")
		sp.add_collider(1, 0.75, 0.15, 1.5)      # Box
		sp.add_physics(1, 1, False, 0)            # Kinematic
		sp.add_script("scripts.kinematic_platform", "KinematicPlatform")
		self._spawned.append(sp)

		# --- Dynamic boxes (stacked pile) ---
		boxes = [
			("Box A1", (-4, 2, 4),   (0.7, 0.7, 0.7),   (15, 0), 0.5),
			("Box A2", (-2.5, 3.5, 3.5), (0.6, 0.6, 0.6), (-20, 0), 0.4),
			("Box A3", (-1, 5, 4.5),   (0.5, 0.5, 0.5),   (40, 5), 0.3),
			("Box A4", (-3, 6.5, 4),   (0.4, 0.4, 0.4),   (-10, 0), 0.2),
		]
		for name, pos, scale, (ry, rz), mass in boxes:
			e = z1.scene.create_entity(name)
			e.transform.location = z1.Vec3(*pos)
			e.transform.rotation = z1.Vec3(0, float(ry), float(rz))
			e.transform.scale = z1.Vec3(*scale)
			e.add_static_mesh("mesh/SM_Cube")
			half = scale[0] * 0.5
			e.add_collider(1, half, half, half)        # Box
			e.add_physics(2, mass, True, 0.05)          # Dynamic
			self._spawned.append(e)

		# --- Dynamic spheres ---
		spheres = [
			("Sphere A", (5, 4, 2),   0.5, 0.5),
			("Sphere B", (6, 5, -1),  0.4, 0.3),
			("Sphere C", (4, 6.5, -3), 0.35, 0.25),
		]
		for name, pos, radius, mass in spheres:
			e = z1.scene.create_entity(name)
			e.transform.location = z1.Vec3(*pos)
			e.transform.scale = z1.Vec3(radius, radius, radius)
			e.add_static_mesh("mesh/SM_Sphere")
			e.add_collider(0, radius, 0, 0)             # Sphere
			e.add_physics(2, mass, True, 0.05)           # Dynamic
			self._spawned.append(e)

		# --- Rolling capsule (log) ---
		log = z1.scene.create_entity("Rolling Log")
		log.transform.location = z1.Vec3(-5, 3, -2)
		log.transform.rotation = z1.Vec3(0, 0, 90)
		log.transform.scale = z1.Vec3(0.5, 1.5, 0.5)
		log.add_static_mesh("mesh/SM_Cylinder")
		log.add_collider(2, 0.35, 0.75, 0)             # Capsule
		log.add_physics(2, 0.8, True, 0.05)             # Dynamic
		self._spawned.append(log)

		# --- Static ramp ---
		ramp = z1.scene.create_entity("Ramp")
		ramp.transform.location = z1.Vec3(5.5, 0.2, -2)
		ramp.transform.rotation = z1.Vec3(0, 0, 30)
		ramp.transform.scale = z1.Vec3(3, 0.2, 1)
		ramp.add_static_mesh("mesh/SM_Cube")
		ramp.add_collider(1, 1.5, 0.1, 0.5)             # Box
		ramp.add_physics(0, 1, False, 0)                 # Static
		self._spawned.append(ramp)

		z1.log_info(f"[PhysDemo] Spawned {len(self._spawned)} physics objects.")

	def on_detach(self):
		pass
