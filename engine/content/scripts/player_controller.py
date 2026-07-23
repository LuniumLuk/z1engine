"""
Player Controller Demo Script
WASD movement + mouse look via event-based input tracking.
Attach to an entity with a CameraComponent for FPS-style controls.
Uses z1 reflected fields: entity.transform (location, rotation, scale).
"""
import z1
import math


class PlayerController(z1.Script):
    """FPS-style player controller using reflected transform fields."""

    MOVE_SPEED = 8.0
    MOUSE_SENSITIVITY = 0.15

    def __init__(self):
        super().__init__()
        self._keys_pressed = set()
        self._yaw = 0.0
        self._pitch = 0.0
        self._prev_mouse_x = -1.0
        self._prev_mouse_y = -1.0
        self._log_timer = 0.0
        self._cursor_captured = False

    def _log(self, msg):
        z1.log_info(f"[PlayerController] {msg}")

    def _capture_cursor(self):
        """Hide and lock cursor to window center."""
        if not self._cursor_captured:
            z1.hide_cursor()
            z1.center_cursor()
            self._cursor_captured = True
            self._prev_mouse_x = -1.0  # reset delta tracking
            self._prev_mouse_y = -1.0
            self._log("Cursor captured (click to release with ESC)")

    def _release_cursor(self):
        """Show cursor again."""
        if self._cursor_captured:
            z1.show_cursor()
            self._cursor_captured = False
            self._log("Cursor released")

    def on_mouse_button_pressed(self, event):
        """Left click inside window → capture cursor."""
        if event.mouse_button == 0:  # left button
            self._capture_cursor()

    def on_key_pressed(self, event):
        code = event.key_code
        self._keys_pressed.add(code)

        # ESC (256) → release cursor
        if code == 256 and self._cursor_captured:
            self._release_cursor()

    def on_key_released(self, event):
        code = event.key_code
        self._keys_pressed.discard(code)

    def on_mouse_moved(self, event):
        if not self._cursor_captured:
            return
        if self._prev_mouse_x < 0:
            self._prev_mouse_x = event.x
            self._prev_mouse_y = event.y
            return

        dx = event.x - self._prev_mouse_x
        dy = event.y - self._prev_mouse_y
        self._prev_mouse_x = event.x
        self._prev_mouse_y = event.y

        if abs(dx) < 1 and abs(dy) < 1:
            return

        self._yaw -= dx * self.MOUSE_SENSITIVITY
        self._pitch -= dy * self.MOUSE_SENSITIVITY
        self._pitch = max(-89.0, min(89.0, self._pitch))

        if self.entity and self.entity.transform:
            self.entity.transform.rotation = z1.Vec3(self._pitch, self._yaw, 0.0)

    def on_attach(self):
        self._log("Attached! Click in the window to capture cursor, ESC to release.")
        z1.register_event_listener(z1.EventType.KeyPressed, self.on_key_pressed)
        z1.register_event_listener(z1.EventType.KeyReleased, self.on_key_released)
        z1.register_event_listener(z1.EventType.MouseMoved, self.on_mouse_moved)
        z1.register_event_listener(z1.EventType.MouseButtonPressed, self.on_mouse_button_pressed)

    def on_start(self):
        if self.entity and self.entity.transform:
            r = self.entity.transform.rotation
            self._yaw = r.y
            self._pitch = r.x
            self._log(
                f"Started! pos=({r.x:.1f},{r.y:.1f},{r.z:.1f}) "
                f"loc=({self.entity.transform.location.x:.1f},"
                f"{self.entity.transform.location.y:.1f},"
                f"{self.entity.transform.location.z:.1f})"
            )

    def on_update(self, delta):
        if not self.entity or not self.entity.transform:
            return

        t = self.entity.transform

        rad = math.radians(self._yaw)
        forward = z1.Vec3(-math.sin(rad), 0.0, -math.cos(rad))
        right = z1.Vec3(math.cos(rad), 0.0, -math.sin(rad))

        move = z1.Vec3(0.0, 0.0, 0.0)
        spd = self.MOVE_SPEED * delta

        # WASD: camera-relative movement
        if 87 in self._keys_pressed:  # W
            move.x += forward.x * spd; move.z += forward.z * spd
        if 83 in self._keys_pressed:  # S
            move.x -= forward.x * spd; move.z -= forward.z * spd
        if 65 in self._keys_pressed:  # A
            move.x -= right.x * spd; move.z -= right.z * spd
        if 68 in self._keys_pressed:  # D
            move.x += right.x * spd; move.z += right.z * spd
        # Space (32) = up, LCTRL (341) = down
        if 32 in self._keys_pressed:
            move.y += spd
        if 341 in self._keys_pressed:
            move.y -= spd

        if move.x != 0.0 or move.y != 0.0 or move.z != 0.0:
            t.location = z1.Vec3(
                t.location.x + move.x,
                t.location.y + move.y,
                t.location.z + move.z
            )

        # Periodic status
        self._log_timer += delta
        if self._log_timer > 3.0:
            self._log_timer = 0.0
            self._log(
                f"Status: loc=({t.location.x:.1f},{t.location.y:.1f},{t.location.z:.1f}) "
                f"rot=({t.rotation.x:.1f},{t.rotation.y:.1f},{t.rotation.z:.1f}) "
                f"captured={self._cursor_captured}"
            )

    def on_destroy(self):
        self._release_cursor()
        self._log("Destroyed.")

    def on_detach(self):
        self._release_cursor()

