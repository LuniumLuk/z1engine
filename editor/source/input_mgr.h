#pragma once

#include "z1engine.h"

using namespace z1;

struct InputManager {
	bool m_right_button_pressed = false;
	bool m_left_button_pressed = false;
	float m_scroll_delta = 0.0f;
	float m_mouse_delta_x = 0.0f, m_mouse_delta_y = 0.0f;
	float m_mouse_last_x = 0.0f, m_mouse_last_y = 0.0f;
	float m_delta_time;

	void on_event(Event& event) {
		auto dispatcher = EventDispatcher(event);
		dispatcher.dispatch<MouseScrollEvent>(BIND_EVENT_FN(InputManager::on_mouse_scrolled));
		dispatcher.dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(InputManager::on_mouse_pressed));
		dispatcher.dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(InputManager::on_mouse_released));
	}

	void update(float delta_time) {
		m_delta_time = delta_time;

		auto [mouse_x, mouse_y] = g_runtime_context.m_input_system->get_mouse_pos();

		m_mouse_delta_x = mouse_x - m_mouse_last_x;
		m_mouse_delta_y = mouse_y - m_mouse_last_y;
		m_mouse_last_x = mouse_x;
		m_mouse_last_y = mouse_y;
	}

	void reset() {
		m_delta_time = 0.0;
		m_mouse_delta_x = 0.0f;
		m_mouse_delta_y = 0.0f;
		m_scroll_delta = 0.0f;
	}

private:
	bool on_mouse_scrolled(MouseScrollEvent& event) {
		m_scroll_delta += event.get_y_offset();
		return false;
	}

	bool on_mouse_pressed(MouseButtonPressedEvent& event) {
		if (event.GetButton() == MOUSE_BUTTON_RIGHT) {
			m_right_button_pressed = true;
		}
		if (event.GetButton() == MOUSE_BUTTON_LEFT) {
			m_left_button_pressed = true;
		}
		return true;
	}

	bool on_mouse_released(MouseButtonReleasedEvent& event) {
		if (event.GetButton() == MOUSE_BUTTON_RIGHT) {
			m_right_button_pressed = false;
		}
		if (event.GetButton() == MOUSE_BUTTON_LEFT) {
			m_left_button_pressed = false;
		}
		return true;
	}
};