#pragma once

#include "event/event.h"

namespace z1 {

	struct API MouseMovedEvent : Event {
		MouseMovedEvent(float x, float y)
			: m_mouse_x(x), m_mouse_y(y) {}

		float get_x() const { return m_mouse_x; }
		float get_y() const { return m_mouse_y; }

		std::string to_string() const override {
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_mouse_x << ", " << m_mouse_y;
			return ss.str();
		}

		EVENT_STRUCT_TYPE(MouseMoved)
		EVENT_STRUCT_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_mouse_x, m_mouse_y;
	};

	struct API MouseScrollEvent : Event {
		MouseScrollEvent(float x, float y)
			: m_x_offset(x), m_y_offset(y) {}

		float get_x_offset() const { return m_x_offset; }
		float get_y_offset() const { return m_y_offset; }

		std::string to_string() const override {
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_x_offset << ", " << m_y_offset;
			return ss.str();
		}

		EVENT_STRUCT_TYPE(MouseScrolled)
		EVENT_STRUCT_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_x_offset, m_y_offset;
	};

	struct API MouseButtonEvent : Event {
		int get_button() const { return m_mouse_button; }

		EVENT_STRUCT_CATEGORY(EventCategoryMouseButton | EventCategoryInput)

	protected:
		MouseButtonEvent(int button)
			: m_mouse_button(button) {}

	private:
		int m_mouse_button;
	};

	struct API MouseButtonPressedEvent : MouseButtonEvent {
		MouseButtonPressedEvent(int button)
			: MouseButtonEvent(button) {}

		std::string to_string() const override {
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << get_button();
			return ss.str();
		}

		EVENT_STRUCT_TYPE(MouseButtonPressed)
	};

	struct API MouseButtonReleasedEvent : MouseButtonEvent {
		MouseButtonReleasedEvent(int button)
			: MouseButtonEvent(button) {}

		std::string to_string() const override {
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << get_button();
			return ss.str();
		}

		EVENT_STRUCT_TYPE(MouseButtonReleased)
	};

}
