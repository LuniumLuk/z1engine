#include "pch.h"
#include "core/input.h"
#include "glfw/glfw3.h"

namespace z1 {

	InputSystem::InputSystem(std::shared_ptr<Window> const& window) {
		window->add_event_callback(BIND_EVENT_FN(InputSystem::on_event));
	}

	bool InputSystem::is_key_pressed(int keycode) const {
		auto window = static_cast<GLFWwindow*>(g_runtime_context.m_window->get_native_window());
		auto state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool InputSystem::is_mouse_button_pressed(int button) const {
		auto window = static_cast<GLFWwindow*>(g_runtime_context.m_window->get_native_window());
		auto state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}

	std::pair<float, float> InputSystem::get_mouse_pos() const {
		auto window = static_cast<GLFWwindow*>(g_runtime_context.m_window->get_native_window());
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		return { (float)x, (float)y };
	}

	void InputSystem::reset() {
		m_scroll_delta = 0.0f;
	}

	void InputSystem::on_event(Event& event) {
		EventDispatcher dispatcher(event);
		dispatcher.dispatch<MouseScrollEvent>(BIND_EVENT_FN(InputSystem::on_mouse_scrolled));
	}

	bool InputSystem::on_mouse_scrolled(MouseScrollEvent& event) {
		m_scroll_delta = event.get_y_offset();
		return false;
	}

	float InputSystem::get_scroll_delta() const {
		return m_scroll_delta;
	}

}
