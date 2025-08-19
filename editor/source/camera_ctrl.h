#pragma once

#include "z1engine.h"
#include "gui.h"

using namespace z1;

struct CameraCtrlScript : ScriptBase {
	CameraCtrlScript(std::shared_ptr<EditorGUI> const& gui)
		: m_gui(gui) {
	}

	void on_attach() override {
		auto [mouse_x, mouse_y] = g_runtime_context.m_input_system->get_mouse_pos();
		m_mouse_last_x = mouse_x;
		m_mouse_last_y = mouse_y;
	}

	bool preprocess() {
		auto& camera = get_component<CameraComponent>();

		if (!camera.m_is_primary) return false;

		auto [mouse_x, mouse_y] = g_runtime_context.m_input_system->get_mouse_pos();
		m_mouse_delta_x = mouse_x - m_mouse_last_x;
		m_mouse_delta_y = mouse_y - m_mouse_last_y;
		m_mouse_last_x = mouse_x;
		m_mouse_last_y = mouse_y;

		if (!m_gui->is_viewport_focused()) return false;

		return true;
	}

	void on_detach() override {}

	float m_mouse_delta_x = 0.0f;
	float m_mouse_delta_y = 0.0f;
	float m_mouse_last_x = 0.0f;
	float m_mouse_last_y = 0.0f;

	std::shared_ptr<EditorGUI> m_gui = nullptr;
};

struct HoveringCameraCtrlScript : CameraCtrlScript {
	HoveringCameraCtrlScript(std::shared_ptr<EditorGUI> const& gui)
		: CameraCtrlScript(gui) {
	}

	void on_update(float delta_time) override {
		if (!preprocess()) return;

		auto& transform = get_component<TransformComponent>();

		if (g_runtime_context.m_input_system->is_mouse_button_pressed(MOUSE_BUTTON_RIGHT)) {
			transform.m_rotation.x += -m_mouse_delta_y * m_rotate_speed * delta_time;
			transform.m_rotation.y += -m_mouse_delta_x * m_rotate_speed * delta_time;
			transform.m_rotation.x = std::clamp(transform.m_rotation.x, -89.0f, 89.0f); // clamp pitch to avoid gimbal lock
		}

		glm::vec3 move(0.0f);
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_A)) {
			move.x -= m_move_speed * delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_D)) {
			move.x += m_move_speed * delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_S)) {
			move.z -= m_move_speed * delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_W)) {
			move.z += m_move_speed * delta_time;
		}

		auto postive_x = transform.get_world_rotation() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		auto postive_z = transform.get_world_rotation() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		move = move.x * glm::vec3(postive_x) + move.z * glm::vec3(postive_z);

		transform.m_location += move;
	}

	float m_rotate_speed = 40.0f;
	float m_move_speed = 4.0f;
};

struct Generic2DCameraCtrlScript : CameraCtrlScript {
	Generic2DCameraCtrlScript(std::shared_ptr<EditorGUI> const& gui)
		: CameraCtrlScript(gui) {
	}

	void on_update(float delta_time) override {
		if (!preprocess()) return;

		auto& transform = get_component<TransformComponent>();
		auto& camera = get_component<CameraComponent>();

		m_drag_speed = m_gui->m_viewport_pixel_scale_x;

		glm::vec3 move(0.0f);
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_A)) {
			move.x -= m_move_speed * delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_D)) {
			move.x += m_move_speed * delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_S)) {
			move.y -= m_move_speed * delta_time;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_W)) {
			move.y += m_move_speed * delta_time;
		}

		camera.zoom(std::exp(m_zoom_speed * g_runtime_context.m_input_system->get_scroll_delta()));

		auto proj = camera.get_proj();
		auto k = 2.0f / proj[0][0];
		if (g_runtime_context.m_input_system->is_mouse_button_pressed(MOUSE_BUTTON_LEFT)) {
			move.x -= m_drag_speed * m_mouse_delta_x * k;
			move.y += m_drag_speed * m_mouse_delta_y * k;
		}

		auto postive_x = transform.get_world_transform() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		auto postive_y = transform.get_world_transform() * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		move = move.x * glm::vec3(postive_x) + move.y * glm::vec3(postive_y);

		transform.m_location += move;
	}

	void on_detach() override {
	}

	float m_drag_speed = 1.0f;
	float m_move_speed = 1.0f;
	float m_zoom_speed = 0.1f;
};
