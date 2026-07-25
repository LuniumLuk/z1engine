#pragma once

#include "core/core.h"
#include <string>

namespace z1 {

	enum struct API EventType : int {
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
	};

	enum API EventCategoryFlag {
		None = 0,
		EventCategoryApplication    = (1 << 0),
		EventCategoryInput          = (1 << 1),
		EventCategoryKeyboard       = (1 << 2),
		EventCategoryMouse          = (1 << 3),
		EventCategoryMouseButton    = (1 << 4),
	};

#define EVENT_STRUCT_TYPE(type) static EventType get_static_type() { return EventType::type; } \
								virtual EventType get_event_type() const override { return get_static_type(); } \
								virtual const char* get_name() const override { return #type; }

#define EVENT_STRUCT_CATEGORY(category) virtual int get_category_flags() const override { return category; }

	struct API Event {
		friend struct EventDispatcher;

		virtual EventType get_event_type() const = 0;
		virtual char const* get_name() const = 0;
		virtual int get_category_flags() const = 0;
		virtual std::string to_string() const { return get_name(); }

		void mark_handled() { m_handled = true; }

		bool is_in_category(EventCategoryFlag category) const {
			return get_category_flags() & category;
		}
		bool is_handled() const { return m_handled; }

	private:
		bool m_handled = false;

	};

	struct API EventDispatcher {
		template<typename T>
		using EventFn = std::function<bool(T&)>;

		EventDispatcher(Event& event)
			: m_event(event) {}

		template<typename T>
		bool dispatch(EventFn<T> func) {
			if (m_event.get_event_type() == T::get_static_type()) {
				m_event.m_handled = func(static_cast<T&>(m_event));
				return true;
			}
			return false;
		}
	private:
		Event& m_event;
	};

	inline API std::ostream& operator<<(std::ostream& os, Event const& e) {
		return os << e.to_string();
	}

}
