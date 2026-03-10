#pragma once

#include "event/event.h"

namespace z1 {

	struct API KeyEvent : Event {
		int get_keycode() const { return m_keycode; }

		EVENT_STRUCT_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	protected:
		KeyEvent(int keycode)
			: m_keycode(keycode) {}

	private:
		int m_keycode;
	};

	struct API KeyPressedEvent : KeyEvent {
		KeyPressedEvent(int keycode, int repeat)
			: KeyEvent(keycode), m_repeat_count(repeat) {}

		int get_repeat_count() const { return m_repeat_count; }

		std::string to_string() const override {
			std::stringstream ss;
			ss << "KeyPressedEvent: " << get_keycode() << " (" << m_repeat_count << " repeats)";
			return ss.str();
		}

		EVENT_STRUCT_TYPE(KeyPressed)

	private:
		int m_repeat_count;
	};

	struct API KeyReleasedEvent : KeyEvent {
		KeyReleasedEvent(int keycode, int repeat)
			: KeyEvent(keycode) {}

		std::string to_string() const override {
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << get_keycode();
			return ss.str();
		}

		EVENT_STRUCT_TYPE(KeyReleased)
	};

	struct API KeyTypedEvent : KeyEvent {
		KeyTypedEvent(int keycode)
			: KeyEvent(keycode) {}

		std::string to_string() const override {
			std::stringstream ss;
			ss << "KeyTypedEvent: " << get_keycode();
			return ss.str();
		}

		EVENT_STRUCT_TYPE(KeyTyped)
	};

}
