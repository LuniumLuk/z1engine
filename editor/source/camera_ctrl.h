#pragma once

#include "z1engine.h"
#include "input_mgr.h"

using namespace z1;

struct CameraController {
	CameraController::CameraController(std::shared_ptr<Entity> const& camera) : m_camera(camera) {}

	std::shared_ptr<Entity> const& get_camera() const { return m_camera; }

	virtual void update(InputManager const& state) = 0;

protected:
	std::shared_ptr<Entity> m_camera;
};

struct API HoveringCameraController : CameraController {
	HoveringCameraController(std::shared_ptr<Entity> const& camera)
		: CameraController(camera) {
	}

	void update(InputManager const& state) override {
		/*if (state.m_right_button_pressed) {
			m_camera->rotate(
				state.m_mouse_delta_y * m_rotate_speed * (float)state.m_delta_time,
				state.m_mouse_delta_x * m_rotate_speed * (float)state.m_delta_time,
				0.0f);
		}

		glm::vec3 move(0.0f);
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_A)) {
			move.x -= m_move_speed * (float)state.m_delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_D)) {
			move.x += m_move_speed * (float)state.m_delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_S)) {
			move.z -= m_move_speed * (float)state.m_delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_W)) {
			move.z += m_move_speed * (float)state.m_delta_time;
		}

		m_camera->move(move);*/
	}

	float m_rotate_speed = 1.0f;
	float m_move_speed = 1.0f;
};

struct API OrbitingCameraController : CameraController {
	OrbitingCameraController(std::shared_ptr<Entity> const& camera)
		: CameraController(camera) {
	}

	void update(InputManager const& state) override {
		UNIMPLEMENTED_FUNCTION();
	}
};

struct API Generic2DCameraController : CameraController {
	Generic2DCameraController(std::shared_ptr<Entity> const& camera)
		: CameraController(camera) {
	}

	void update(InputManager const& state) override {
		//glm::vec3 move(0.0f);
		//if (g_runtime_context.m_input_system->is_key_pressed(KEY_A)) {
		//	move.x -= m_move_speed * (float)state.m_delta_time;
		//}
		//if (g_runtime_context.m_input_system->is_key_pressed(KEY_D)) {
		//	move.x += m_move_speed * (float)state.m_delta_time;
		//}
		//if (g_runtime_context.m_input_system->is_key_pressed(KEY_S)) {
		//	move.y -= m_move_speed * (float)state.m_delta_time;
		//}
		//if (g_runtime_context.m_input_system->is_key_pressed(KEY_W)) {
		//	move.y += m_move_speed * (float)state.m_delta_time;
		//}

		//auto proj = m_camera->get_proj();
		//auto k = 2.0f / proj[0][0];
		//if (state.m_left_button_pressed) {
		//	move.x -= m_drag_speed * state.m_mouse_delta_x * k;
		//	move.y += m_drag_speed * state.m_mouse_delta_y * k;
		//}

		//m_camera->move(move);
		//m_camera->zoom(std::exp(m_zoom_speed * state.m_scroll_delta));
	}

	float m_drag_speed = 1.0f;
	float m_move_speed = 1.0f;
	float m_zoom_speed = 0.1f;
};
