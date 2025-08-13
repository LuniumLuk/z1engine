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
		auto& transform = m_camera->get_component<TransformComponent>();
		if (state.m_right_button_pressed) {
			transform.m_rotation.x += -state.m_mouse_delta_y * m_rotate_speed * (float)state.m_delta_time;
			transform.m_rotation.y += -state.m_mouse_delta_x * m_rotate_speed * (float)state.m_delta_time;
			transform.m_rotation.x = std::clamp(transform.m_rotation.x, -89.0f, 89.0f); // clamp pitch to avoid gimbal lock
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

		auto postive_x = transform.get_world_rotation() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		auto postive_z = transform.get_world_rotation() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		move = move.x * glm::vec3(postive_x) + move.z * glm::vec3(postive_z);

		transform.m_location += move;
	}

	float m_rotate_speed = 40.0f;
	float m_move_speed = 4.0f;
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
		glm::vec3 move(0.0f);
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_A)) {
			move.x -= m_move_speed * (float)state.m_delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_D)) {
			move.x += m_move_speed * (float)state.m_delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_S)) {
			move.y -= m_move_speed * (float)state.m_delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_W)) {
			move.y += m_move_speed * (float)state.m_delta_time;
		}

		auto& camera = m_camera->get_component<CameraComponent>();
		camera.zoom(std::exp(m_zoom_speed * state.m_scroll_delta));

		auto proj = camera.get_proj();
		auto k = 2.0f / proj[0][0];
		if (state.m_left_button_pressed) {
			move.x -= m_drag_speed * state.m_mouse_delta_x * k;
			move.y += m_drag_speed * state.m_mouse_delta_y * k;
		}

		auto& transform = m_camera->get_component<TransformComponent>();

		auto postive_x = transform.get_world_transform() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		auto postive_y = transform.get_world_transform() * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		move = move.x * glm::vec3(postive_x) + move.y * glm::vec3(postive_y);

		transform.m_location += move;
	}

	float m_drag_speed = 1.0f;
	float m_move_speed = 1.0f;
	float m_zoom_speed = 0.1f;
};
